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

/* Days in Jalali month */
static int jalali_month_days(int m) {
    return (m <= 6) ? 31 : ((m <= 11) ? 30 : 29);
}

/* Days in Gregorian month */
static int gregorian_month_days(int y, int m) {
    static const int md[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2) {
        if ((y % 400 == 0) || ((y % 4 == 0) && (y % 100 != 0)))
            return 29;
    }
    if (m >= 1 && m <= 12)
        return md[m-1];
    return 0;
}

/* Convert Gregorian to Jalali */
static date_struct gregorian_to_jalali_core(int gy, int gm, int gd) {
    date_struct res;
    int gy2 = gy - 1600;
    int gm2 = gm - 1;
    int gd2 = gd - 1;

    long g_day_no = 365L * gy2 + (gy2 + 3) / 4 - (gy2 + 99) / 100 + (gy2 + 399) / 400;
    for (int i = 0; i < gm2; ++i)
        g_day_no += gregorian_month_days(gy, i+1);
    g_day_no += gd2;

    long j_day_no = g_day_no - 79;
    long j_np = j_day_no / 12053;
    j_day_no %= 12053;

    long jy = 979 + 33 * j_np + 4 * (j_day_no / 1461);
    j_day_no %= 1461;

    if (j_day_no >= 366) {
        jy += (j_day_no - 1) / 365;
        j_day_no = (j_day_no - 1) % 365;
    }

    int jm = 0;
    int jd = 0;
    for (int i = 1; i <= 12; ++i) {
        int dim = jalali_month_days(i);
        if (j_day_no < dim) {
            jm = i;
            jd = j_day_no + 1;
            break;
        }
        j_day_no -= dim;
    }

    res.year = (int)jy;
    res.month = jm;
    res.day = jd;
    return res;
}

/* Convert Jalali to Gregorian */
static date_struct jalali_to_gregorian_core(int jy, int jm, int jd) {
    date_struct res;
    int jy2 = jy - 979;
    int jm2 = jm - 1;
    int jd2 = jd - 1;

    long j_day_no = 365L * jy2 + (jy2 / 33) * 8 + ((jy2 % 33 + 3) / 4);
    for (int i = 0; i < jm2; ++i)
        j_day_no += jalali_month_days(i+1);
    j_day_no += jd2;

    long g_day_no = j_day_no + 79;
    long gy = 1600 + 400 * (g_day_no / 146097);
    g_day_no %= 146097;

    int leap = 1;
    if (g_day_no >= 36525) {
        g_day_no--;
        gy += 100 * (g_day_no / 36524);
        g_day_no %= 36524;
        if (g_day_no >= 365)
            g_day_no++;
        else
            leap = 0;
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
        int dim = gregorian_month_days((int)gy, i);
        if (i == 2 && !leap)
            dim = 28;
        else if (i == 2 && leap)
            dim = 29;
        if (g_day_no < dim) {
            gm = i;
            gd = g_day_no + 1;
            break;
        }
        g_day_no -= dim;
    }

    res.year = (int)gy;
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
        y < 1 || y > 9999 || m < 1 || m > 12 || d < 1 || d > gregorian_month_days(y, m)) {
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
        y < 1 || y > 9999 || m < 1 || m > 12 || d < 1 || d > jalali_month_days(m)) {
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
