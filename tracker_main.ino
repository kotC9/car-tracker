#include <DS3231.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

enum system_parts
{
  sys_module,
  gsm_module,
  rtc_module,
  sd_module,

  module_count = 4
};

#define IDNAME(name) #name
const String modules_name[] = {IDNAME(sys_module), IDNAME(gsm_module), IDNAME(rtc_module), IDNAME(sd_module)};
const int sd_cs = 4;
const String datetime_filename = "datetime.txt";

bool module_ok[] = {false, false, false, false};
File log_file;
File gsm_track;
DS3231 rtc;

String date_format(int val)
{
  return (val < 10 ? "0" : "") + String(val);
}

String get_datetime()
{
  if(!module_ok[rtc_module])
  {
    return "";
  }
  else
  {
    bool unused;

    return String("20" + date_format(rtc.getYear()) + "-" + date_format(rtc.getMonth(unused)) + "-" + date_format(rtc.getDate()) + " " + \
                  date_format(rtc.getHour(unused,unused)) + ":" + date_format(rtc.getMinute()) + ":" + date_format(rtc.getSecond()));
  }
}

bool try_set_new_datetime(String date)
{
  // date format: "2022-12-31 23:59:59"
  if(date.length() < 19)
  {
    return false;
  }

  int year = date.substring(0, 4).toInt() - 2000;
  byte month = date.substring(5, 7).toInt();
  byte day = date.substring(8, 10).toInt();
  byte hour = date.substring(11, 13).toInt();
  byte minute = date.substring(14, 16).toInt();
  byte second = date.substring(17, 19).toInt();

  rtc.setYear(year);
  rtc.setMonth(month);
  rtc.setDate(day);
  rtc.setHour(hour);
  rtc.setMinute(minute);
  rtc.setSecond(second);

  return true;
}

void sd_println(File *file, String str)
{
  if(module_ok[sd_module])
  {
    file->println(str);
    file->flush();
  }
}

void log(system_parts part, String text)
{
  String name = modules_name[part];
  String date = get_datetime();
  String str = String("[" + name + "][" + date + "]: " + text);
  Serial.println(str);
  sd_println(&log_file, str);
}

bool sd_init()
{
  if(!SD.begin(sd_cs))
  {
    log(sd_module, "begin failed");
    return false;
  }

  log_file = SD.open("log.txt", O_APPEND);

  log(sd_module, "init ok");
  return true;
}

bool rtc_init()
{
  Wire.begin();

  rtc.setClockMode(false);

  // если сдкарта подключена - пытаемся считать новую дату
  if(module_ok[sd_module])
  {
    if(SD.exists(datetime_filename))
    {
        File dt_file = SD.open(datetime_filename);
        if(dt_file)
        {
          log(rtc_module, "reading datetime from file");
          String new_datetime = "";
          while (dt_file.available()) {
            new_datetime += (char)dt_file.read();
          }
          log(rtc_module, "file content: " + new_datetime);
          if(try_set_new_datetime(new_datetime))
          {
            log(rtc_module, "done reading");
            log(rtc_module, "new datetime: " + new_datetime);
            dt_file.close();
            SD.remove(datetime_filename);
          }
          else
          {
            log(rtc_module, "error reading(incorrect format or others)");
          }
        }
    }
  }

  log(rtc_module, "init ok");
  return true;
}

bool gsm_init()
{

  log(gsm_module, "init ok");
  return true;
}

void setup() {
  Serial.begin(9600);
  log(sys_module, "start");

  
  // sd module must be initied first
  module_ok[sys_module] = true;
  module_ok[sd_module] = sd_init();
  module_ok[rtc_module] = rtc_init();
  module_ok[gsm_module] = gsm_init();
}

void loop() {

}
