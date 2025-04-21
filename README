# MySQL 8+ Plugin for Gregorian to Jalali Date Conversion
### افزونه‌ای برای MySQL 8+ جهت تبدیل تاریخ میلادی به شمسی


### پیش‌نیازها

1. **نصب ابزارهای توسعه**:
   - در سیستم‌عامل اوبونتو (Ubuntu)، نیاز به نصب ابزارهای توسعه مانند `gcc`, `make` و `mysql-devel` دارید:
```bash
sudo apt update
sudo apt install build-essential libmysqlclient-dev
```

### مراحل کامپایل و نصب:

1. از پروژه clone بگیرید و به ریشه بروید.
2.دستور ```make``` را اجرا کنید.
3.فایل ساخته شده در پوشه بیلد را  (با فرمت .so) به پوشه پلاگین های mysql اضافه کنید.
```
sudo cp build/linux/x86_64/gregorian_to_jalali.so /usr/lib64/mysql/plugin/
```

4.بارگذاری پلاگین در MySQL، برای بارگذاری پلاگین در MySQL، دستور زیر را اجرا کنید:
```
CREATE FUNCTION gregorian_to_jalali
RETURNS STRING
SONAME 'gregorian_to_jalali.so';

CREATE FUNCTION jalali_to_gregorian
RETURNS STRING
SONAME 'gregorian_to_jalali.so';
```

### روش استفاده:
```
select created_at,
       gregorian_to_jalali(created_at),
       jalali_to_gregorian(gregorian_to_jalali(created_at))
    from my_table;
```

### حذف پلاگین (در صورت نیاز)، اگر نیاز به حذف پلاگین داشتید، می‌توانید از دستور زیر استفاده کنید:
```
DROP FUNCTION gregorian_to_jalali;
DROP FUNCTION jalali_to_gregorian;
```
