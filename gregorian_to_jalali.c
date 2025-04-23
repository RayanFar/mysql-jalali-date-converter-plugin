#include <mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef my_bool
#define my_bool int
#endif

#define DATE_STR_LEN 10    /* "YYYY-MM-DD" */
#define DATE_BUF_LEN (DATE_STR_LEN + 1)

typedef struct { int year, month, day; } date_struct;

/* Leap-year check for Gregorian calendar */
static int is_gregorian_leap(int y) {
    return ( (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0) );
}

/* Days in each Gregorian month */
static int days_in_gregorian_month(int y, int m) {
    static const int md[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && is_gregorian_leap(y))
        return 29;
    if (m >= 1 && m <= 12)
        return md[m-1];
    return 0;
}

/* Days in each Jalali month */
static int days_in_jalali_month(int m) {
    static const int jd[12] = {31,31,31,31,31,31,30,30,30,30,30,29};
    if (m >= 1 && m <= 12)
        return jd[m-1];
    return 0;
}

/* Convert Gregorian to Jalali */
date_struct gregorian_to_jalali_core(int gy, int gm, int gd) {
    date_struct res;
    int g_day_no = 365 * (gy - 1) + (gy - 1)/4 - (gy - 1)/100 + (gy - 1)/400;
    for (int i = 1; i < gm; ++i)
        g_day_no += days_in_gregorian_month(gy, i);
    g_day_no += gd - 1;

    int j_day_no = g_day_no - 79;
    int j_np = j_day_no / 12053;
    j_day_no %= 12053;

    int jy = 979 + 33 * j_np + 4 * (j_day_no / 1461);
    j_day_no %= 1461;

    if (j_day_no >= 366) {
        jy += (j_day_no - 1) / 365;
        j_day_no = (j_day_no - 1) % 365;
    }

    int jm = 0;
    int jd_day = 0;
    for (int i = 1; i <= 12; ++i) {
        int dim = days_in_jalali_month(i);
        if (j_day_no < dim) {
            jm = i;
            jd_day = j_day_no + 1;
            break;
        }
        j_day_no -= dim;
    }

    res.year = jy;
    res.month = jm;
    res.day = jd_day;
    return res;
}

/* Convert Jalali to Gregorian */
date_struct jalali_to_gregorian_core(int jy, int jm, int jd) {
    date_struct res;
    int j_day_no = (jy - 979) * 365 + ((jy - 979)/33) * 8 + (((jy - 979) % 33 + 3)/4);
    for (int i = 1; i < jm; ++i)
        j_day_no += days_in_jalali_month(i);
    j_day_no += jd - 1;

    int g_day_no = j_day_no + 79;
    int gy = 1600 + 400 * (g_day_no / 146097);
    g_day_no %= 146097;
    int leap = 1;
    if (g_day_no >= 36525) {
        g_day_no--;
        gy += 100 * (g_day_no / 36524);
        g_day_no %= 36524;
        if (g_day_no >= 365) {
            g_day_no++;
        } else {
            leap = 0;
        }
    }
    gy += 4 * (g_day_no / 1461);
    g_day_no %= 1461;
    if (g_day_no >= 366) {
        leap = 0;
        g_day_no--;
        gy += g_day_no / 365;
        g_day_no %= 365;
    }

    int gm = 0;
    int gd = 0;
    for (int i = 1; i <= 12; ++i) {
        int dim = days_in_gregorian_month(gy, i) + (i == 2 && leap);
        if (g_day_no < dim) {
            gm = i;
            gd = g_day_no + 1;
            break;
        }
        g_day_no -= dim;
    }

    res.year = gy;
    res.month = gm;
    res.day = gd;
    return res;
}

/* UDF: gregorian_to_jalali */
my_bool gregorian_to_jalali_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    if (args->arg_count != 1 || args->arg_type[0] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "gregorian_to_jalali: Expected one DATE string argument 'YYYY-MM-DD'");
        return 1;
    }
    initid->max_length = DATE_STR_LEN;
    return 0;
}

char *gregorian_to_jalali(UDF_INIT *initid, UDF_ARGS *args,
                           char *result, unsigned long *length,
                           char *is_null, char *error) {
    int y, m, d;
    if (sscanf(args->args[0], "%d-%d-%d", &y, &m, &d) != 3 ||
        y < 1 || y > 9999 || m < 1 || m > 12 || d < 1 || d > days_in_gregorian_month(y, m)) {
        *is_null = 1;
        return NULL;
    }
    date_struct out = gregorian_to_jalali_core(y, m, d);
    snprintf(result, DATE_BUF_LEN, "%04d-%02d-%02d",
             out.year, out.month, out.day);
    *length = strlen(result);
    return result;
}

void gregorian_to_jalali_deinit(UDF_INIT *initid) { /* no cleanup needed */ }

/* UDF: jalali_to_gregorian */
my_bool jalali_to_gregorian_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    if (args->arg_count != 1 || args->arg_type[0] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "jalali_to_gregorian: Expected one DATE string argument 'YYYY-MM-DD'");
        return 1;
    }
    initid->max_length = DATE_STR_LEN;
    return 0;
}

char *jalali_to_gregorian(UDF_INIT *initid, UDF_ARGS *args,
                           char *result, unsigned long *length,
                           char *is_null, char *error) {
    int y, m, d;
    if (sscanf(args->args[0], "%d-%d-%d", &y, &m, &d) != 3 ||
        y < 1 || y > 9999 || m < 1 || m > 12 || d < 1 || d > days_in_jalali_month(m)) {
        *is_null = 1;
        return NULL;
    }
    date_struct out = jalali_to_gregorian_core(y, m, d);
    snprintf(result, DATE_BUF_LEN, "%04d-%02d-%02d",
             out.year, out.month, out.day);
    *length = strlen(result);
    return result;
}

void jalali_to_gregorian_deinit(UDF_INIT *initid) { /* no cleanup needed */ }
