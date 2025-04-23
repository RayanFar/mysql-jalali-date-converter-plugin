#include <mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef my_bool
#define my_bool int
#endif

#define DATE_STR_LEN 10  /* "YYYY-MM-DD" length without null */
#define DATE_BUF_LEN  (DATE_STR_LEN + 1)

/*------------ Helper Functions ------------*/
static int is_gregorian_leap(int y) {
    return ( (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0) );
}

static int days_in_gregorian_month(int y, int m) {
    static const int dm[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && is_gregorian_leap(y))
        return 29;
    if (m >= 1 && m <= 12)
        return dm[m];
    return 0;
}


typedef struct { int year, month, day; } date_struct;

date_struct gregorian_to_jalali_core(int gy, int gm, int gd) {
    date_struct result;
    int array[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    if(year <= 1600) {
        year -= 621;
        result.year = 0;
    } else {
        year -= 1600;
        result.year = 979;
    }

    int temp = (year > 2) ? (year + 1) : year;
    int days = ((int)((temp + 3) / 4)) + (365 * year) - ((int)((temp + 99) / 100)) - 80 + array[month - 1] + ((int)((temp + 399) / 400)) + day;
    result.year += 33 * ((int)(days / 12053));
    days %= 12053;
    result.year += 4 * ((int)(days / 1461));
    days %= 1461;

    if(days > 365){
        result.year += (int)((days - 1) / 365);
        days = (days-1) % 365;
    }

    result.month = (days < 186)
                   ? 1 + (int)(days / 31)
                   : 7 + (int)((days - 186) / 30);

    result.day = 1 + ((days < 186)
                      ? (days % 31)
                      : ((days - 186) % 30));
    return result;
}

date_struct jalali_to_gregorian_core(int jy, int jm, int jd) {
    date_struct result;

    int gy, gm, gd;
    int jy = year - 979;
    int jm = month - 1;
    int jd = day - 1;

    int j_day_no = 365 * jy + (jy / 33) * 8 + ((jy % 33 + 3) / 4);
    for (int i = 0; i < jm; ++i)
        j_day_no += (i < 6) ? 31 : 30;
    j_day_no += jd;

    int g_day_no = j_day_no + 79;

    gy = 1600 + 400 * (g_day_no / 146097);
    g_day_no = g_day_no % 146097;

    int leap = 1;
    if (g_day_no >= 36525) {
        g_day_no--;
        gy += 100 * (g_day_no / 36524);
        g_day_no = g_day_no % 36524;

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
        g_day_no = g_day_no % 365;
    }

    int days[] = {0, 31, (leap ? 29 : 28), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (gm = 1; gm <= 12 && g_day_no >= days[gm]; gm++)
        g_day_no -= days[gm];
    gd = g_day_no + 1;

    result.year = gy;
    result.month = gm;
    result.day = gd;

    return result;
}


my_bool gregorian_to_jalali_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    if (args->arg_count != 1 || args->arg_type[0] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "gregorian_to_jalali: Expected one DATE string argument in 'YYYY-MM-DD' format");
        return 1;
    }
    initid->max_length = DATE_STR_LEN;
    return 0;
}

char *gregorian_to_jalali(UDF_INIT *initid, UDF_ARGS *args,
                           char *result, unsigned long *length,
                           char *is_null, char *error) {
    int year, month, day;
    if (sscanf(args->args[0], "%d-%d-%d", &year, &month, &day) != 3) {
        *is_null = 1;
        return NULL;
    }
    /* Validate ranges */
    if (year < 1 || year > 9999 || month < 1 || month > 12 ||
        day < 1 || day > days_in_gregorian_month(year, month)) {
        *is_null = 1;
        return NULL;
    }

    date_struct res = gregorian_to_jalali_core(year, month, day);
    snprintf(result, DATE_BUF_LEN, "%04d-%02d-%02d", res.year, res.month, res.day);
    *length = strlen(result);
    return result;
}

void gregorian_to_jalali_deinit(UDF_INIT *initid) { }


my_bool jalali_to_gregorian_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    if (args->arg_count != 1 || args->arg_type[0] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "jalali_to_gregorian: Expected one DATE string argument in 'YYYY-MM-DD' format");
        return 1;
    }
    initid->max_length = DATE_STR_LEN;
    return 0;
}

char *jalali_to_gregorian(UDF_INIT *initid, UDF_ARGS *args,
                           char *result, unsigned long *length,
                           char *is_null, char *error) {
    int year, month, day;
    if (sscanf(args->args[0], "%d-%d-%d", &year, &month, &day) != 3) {
        *is_null = 1;
        return NULL;
    }
    if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 || day > 31) {
        *is_null = 1;
        return NULL;
    }

    date_struct res = jalali_to_gregorian_core(year, month, day);
    snprintf(result, DATE_BUF_LEN, "%04d-%02d-%02d", res.year, res.month, res.day);
    *length = strlen(result);
    return result;
}

void jalali_to_gregorian_deinit(UDF_INIT *initid) { /* no-op */ }
