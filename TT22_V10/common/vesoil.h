enum SensorCapability
{
    GPS        = 1,
    Distance   = 2,
    Moist1     = 4,
    Moist2     = 8,
    AirTempHum = 16,
    GndTemp    = 32
};

struct SensorId
{
    unsigned char id[6];
};


struct SensorGps
{
    long time;
    float lat;
    float lng; 
    float alt;
    char sats;
    char hdop;
};

struct SensorTempId
{
    byte id=2;
    SensorGps data;
};

struct SensorTemp
{
    float value;
};

struct SensorVoltage
{
    float value;
};

struct SensorMoisture
{
    int value;
};

struct SensorHumidity
{
    float value;
};

struct SensorDistance
{
    float value;
};

struct SensorAirTmp
{
    SensorTemp airtemp;
    SensorHumidity airhum;
};

struct SensorReport
{
    char version=2;
    byte capability;
    SensorVoltage volts;
    SensorId id;
    SensorGps gps;
    SensorAirTmp airTempHumidity;
    SensorTemp gndTemp;
    SensorMoisture moist1;
    SensorMoisture moist2;
    SensorDistance distance;
};

//  | ModuleID | Metric ID | Metric Data.. | Metric ID | Metric Data.. | Metric ID | Metric Data.. | 
//  | ModuleID | Metric ID | Metric Data.. | Metric ID | Metric Data.. | Metric ID | Metric Data.. | 

enum SensorMetricType
{
    SMT_GPS        = 1,
    SMT_Distance   = 2,
    SMT_Moist      = 4,
    SMT_AirTemp    = 5,
    SMT_AirHum     = 6,
    SMT_GndTemp    = 7,
};

struct SensorMetric
{
    byte metric;
    byte index;         // may be used if we have multiple moisture sensors
};
