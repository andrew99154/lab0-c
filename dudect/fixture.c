/** dude, is my code constant time?
 *
 * This file measures the execution time of a given function many times with
 * different inputs and performs a Welch's t-test to determine if the function
 * runs in constant time or not. This is essentially leakage detection, and
 * not a timing attack.
 *
 * Notes:
 *
 *  - the execution time distribution tends to be skewed towards large
 *    timings, leading to a fat right tail. Most executions take little time,
 *    some of them take a lot. We try to speed up the test process by
 *    throwing away those measurements with large cycle count. (For example,
 *    those measurements could correspond to the execution being interrupted
 *    by the OS.) Setting a threshold value for this is not obvious; we just
 *    keep the x% percent fastest timings, and repeat for several values of x.
 *
 *  - the previous observation is highly heuristic. We also keep the uncropped
 *    measurement time and do a t-test on that.
 *
 *  - we also test for unequal variances (second order test), but this is
 *    probably redundant since we're doing as well a t-test on cropped
 *    measurements (non-linear transform)
 *
 *  - as long as any of the different test fails, the code will be deemed
 *    variable time.
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../console.h"
#include "../random.h"

#include "constant.h"
#include "fixture.h"
#include "ttest.h"

#define ENOUGH_MEASURE 10000
#define TEST_TRIES 10

static t_context_t **t_array;

/* threshold values for Welch's t-test */
enum {
    t_threshold_bananas = 500, /* Test failed with overwhelming probability */
    t_threshold_moderate = 10, /* Test failed */
};

static void __attribute__((noreturn)) die(void)
{
    exit(111);
}

static void differentiate(int64_t *exec_times,
                          const int64_t *before_ticks,
                          const int64_t *after_ticks)
{
    for (size_t i = 0; i < N_MEASURES; i++)
        exec_times[i] = after_ticks[i] - before_ticks[i];
}

static int cmp_double(const void *a, const void *b)
{
    double da = *(const double *) a;
    double db = *(const double *) b;
    return (da < db) ? -1 : (da > db) ? 1 : 0;
}

static int cmp(const int64_t *a, const int64_t *b)
{
    if (*a == *b)
        return 0;
    return (*a > *b) ? 1 : -1;
}

static double trimmed_mean(double *vals, size_t count)
{
    // calculate the mean of multiple t-values under different percentile
    // sort and crop the extreme value
    qsort(vals, count, sizeof(double), cmp_double);
    size_t trim = (size_t) (count * 0.1);
    size_t start = trim;
    size_t end = count - trim;
    if (end <= start) {
        start = 0;
        end = count;
    }
    double sum = 0.0;
    for (size_t i = start; i < end; i++) {
        sum += vals[i];
    }
    return sum / (end - start);
}

static void update_statistics(const int64_t *exec_times,
                              uint8_t *classes,
                              int64_t *percentiles)
{
    for (size_t i = 0; i < N_MEASURES; i++) {
        int64_t difference = exec_times[i];
        /* CPU cycle counter overflowed or dropped measurement */
        if (difference <= 0)
            continue;

        /* do a t-test on the execution time */
        // raw data, did not cropping
        t_push(t_array[0], difference, classes[i]);

        for (size_t j = 0; j < N_PERCENTILES; j++) {
            if (difference <= percentiles[j])
                t_push(t_array[j + 1], difference, classes[i]);
        }
    }
}

static bool report(void)
{
    // double max_t = fabs(t_compute(t));
    double number_traces = t_array[0]->n[0] + t_array[0]->n[1];
    // double max_tau = max_t / sqrt(number_traces_max_t);

    printf("\033[A\033[2K");
    printf("meas: %7.2lf M, ", (number_traces / 1e6));
    if (number_traces < ENOUGH_MEASURE) {
        printf("not enough measurements (%.0f still to go).\n",
               ENOUGH_MEASURE - number_traces);
        return false;
    }

    double t_values[N_PERCENTILES + 1];
    for (size_t i = 0; i < N_PERCENTILES + 1; i++) {
        t_values[i] = fabs(t_compute(t_array[i]));
    }

    double mean_t = trimmed_mean(t_values, N_PERCENTILES + 1);
    double tau = mean_t / sqrt(number_traces);

    /* max_t: the t statistic value
     * max_tau: a t value normalized by sqrt(number of measurements).
     *          this way we can compare max_tau taken with different
     *          number of measurements. This is sort of "distance
     *          between distributions", independent of number of
     *          measurements.
     * (5/tau)^2: how many measurements we would need to barely
     *            detect the leak, if present. "barely detect the
     *            leak" = have a t value greater than 5.
     */
    printf("trimmed mean t: %+7.2f, tau: %.2e, (5/tau)^2: %.2e.\n", mean_t, tau,
           (double) (5 * 5) / (double) (tau * tau));

    /* Definitely not constant time */
    if (mean_t > t_threshold_bananas)
        return false;

    /* Probably not constant time. */
    if (mean_t > t_threshold_moderate)
        return false;

    /* For the moment, maybe constant time. */
    return true;
}


static int64_t percentile(int64_t *a_sorted, double which, size_t size)
{
    size_t array_position = (size_t) ((double) size * (double) which);
    assert(array_position < size);
    return a_sorted[array_position];
}

static void prepare_percentiles(int64_t *exec_times, int64_t *percentiles)
{
    qsort(exec_times, N_MEASURES, sizeof(int64_t),
          (int (*)(const void *, const void *)) cmp);
    for (size_t i = 0; i < N_PERCENTILES; i++) {
        percentiles[i] = percentile(
            exec_times, 1 - (pow(0.5, 10 * (double) (i + 1) / N_PERCENTILES)),
            N_MEASURES);
    }
}

static bool doit(int mode)
{
    int64_t *before_ticks = calloc(N_MEASURES + 1, sizeof(int64_t));
    int64_t *after_ticks = calloc(N_MEASURES + 1, sizeof(int64_t));
    int64_t *exec_times = calloc(N_MEASURES, sizeof(int64_t));
    uint8_t *classes = calloc(N_MEASURES, sizeof(uint8_t));
    uint8_t *input_data = calloc(N_MEASURES * CHUNK_SIZE, sizeof(uint8_t));
    int64_t *percentiles = calloc(N_PERCENTILES, sizeof(int64_t));

    if (!before_ticks || !after_ticks || !exec_times || !classes ||
        !input_data) {
        die();
    }

    prepare_inputs(input_data, classes);

    bool ret = measure(before_ticks, after_ticks, input_data, mode);
    differentiate(exec_times, before_ticks, after_ticks);

    prepare_percentiles(exec_times, percentiles);
    update_statistics(exec_times, classes, percentiles);

    ret &= report();

    free(before_ticks);
    free(after_ticks);
    free(exec_times);
    free(classes);
    free(input_data);
    free(percentiles);

    return ret;
}

static void init_once(void)
{
    init_dut();
    for (size_t i = 0; i < N_PERCENTILES + 1; i++)
        t_init(t_array[i]);
}

static bool test_const(char *text, int mode)
{
    bool result = false;
    t_array = malloc(sizeof(t_context_t *) * (N_PERCENTILES + 1));

    for (size_t i = 0; i < N_PERCENTILES + 1; i++)
        t_array[i] = malloc(sizeof(t_context_t));

    for (int cnt = 0; cnt < TEST_TRIES; ++cnt) {
        printf("Testing %s...(%d/%d)\n\n", text, cnt, TEST_TRIES);
        init_once();
        for (int i = 0; i < ENOUGH_MEASURE / (N_MEASURES - DROP_SIZE * 2) + 1;
             ++i)
            result = doit(mode);
        printf("\033[A\033[2K\033[A\033[2K");
        if (result)
            break;
    }

    for (size_t i = 0; i < N_PERCENTILES + 1; i++) {
        free(t_array[i]);
    }
    free(t_array);

    return result;
}

#define DUT_FUNC_IMPL(op)                \
    bool is_##op##_const(void)           \
    {                                    \
        return test_const(#op, DUT(op)); \
    }

#define _(x) DUT_FUNC_IMPL(x)
DUT_FUNCS
#undef _
