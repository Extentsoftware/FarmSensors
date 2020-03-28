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
    SensorCapability capability;
    SensorVoltage volts;
    SensorId id;
    SensorGps gps;
    SensorAirTmp airTempHumidity;
    SensorTemp gndTemp;
    SensorMoisture moist1;
    SensorMoisture moist2;
    SensorDistance distance;
};
