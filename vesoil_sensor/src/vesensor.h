#define SENSOR_SECRET 13212

struct SensorReport
{
    int secret;
    int version;
    float volts;
    double lat;
    double lng;    
    int year;
    int month;
    short day;
    short hour;
    short minute;
    short second;
    double alt;
    int sats;
    int hdop;
    float airtemp;
    float airhum;
    float gndtemp;
    int moist1;
    int moist2;
};