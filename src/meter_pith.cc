/*
 Copyright (C) 2021 Fredrik Öhrström

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"dvparser.h"
#include"meters.h"
#include"meters_common_implementation.h"
#include"wmbus.h"
#include"wmbus_utils.h"

struct MeterPITH : public virtual TempHygroMeter, public virtual MeterCommonImplementation {
    MeterPITH(MeterInfo &mi);

    double currentTemperature(Unit u);
    double currentRelativeHumidity();

private:

    void processContent(Telegram *t);

    double current_temperature_c_ {};
    double average_temperature_1h_c_;
    double average_temperature_24h_c_;
    double current_relative_humidity_rh_ {};
    double average_relative_humidity_1h_rh_ {};
    double average_relative_humidity_24h_rh_ {};
};

MeterPITH::MeterPITH(MeterInfo &mi) :
    MeterCommonImplementation(mi, MeterType::LANSENTH)
{
    setExpectedTPLSecurityMode(TPLSecurityMode::AES_CBC_IV);

    addLinkMode(LinkMode::T1);

    addPrint("current_temperature", Quantity::Temperature,
             [&](Unit u){ return currentTemperature(u); },
             "The current temperature.",
             true, true);

    addPrint("current_relative_humidity", Quantity::RelativeHumidity,
             [&](Unit u){ return currentRelativeHumidity(); },
             "The current relative humidity.",
             true, true);

    addPrint("average_temperature_1h", Quantity::Temperature,
             [&](Unit u){ return convert(average_temperature_1h_c_, Unit::C, u); },
             "The average temperature over the last hour.",
             false, true);

    addPrint("average_relative_humidity_1h", Quantity::RelativeHumidity,
             [&](Unit u){ return average_relative_humidity_1h_rh_; },
             "The average relative humidity over the last hour.",
             false, true);

    addPrint("average_temperature_24h", Quantity::Temperature,
             [&](Unit u){ return convert(average_temperature_24h_c_, Unit::C, u); },
             "The average temperature over the last 24 hours.",
             false, true);

    addPrint("average_relative_humidity_24h", Quantity::RelativeHumidity,
             [&](Unit u){ return average_relative_humidity_24h_rh_; },
             "The average relative humidity over the last 24 hours.",
             false, true);
}

shared_ptr<TempHygroMeter> createPiTH(MeterInfo &mi)
{
    return shared_ptr<TempHygroMeter>(new MeterPITH(mi));
}

double MeterPITH::currentTemperature(Unit u)
{
    assertQuantity(u, Quantity::Temperature);
    return convert(current_temperature_c_, Unit::C, u);
}

double MeterPITH::currentRelativeHumidity()
{
    return current_relative_humidity_rh_;
}

void MeterPITH::processContent(Telegram *t)
{
    int offset;
    string key;

    if (findKey(MeasurementType::Unknown, ValueInformation::ExternalTemperature, 0, 0, &key, &t->values))
    {
        extractDVdouble(&t->values, key, &offset, &current_temperature_c_);
        t->addMoreExplanation(offset, " current temperature (%f C)", current_temperature_c_);
    }

    if (findKey(MeasurementType::Unknown, ValueInformation::ExternalTemperature, 1, 0, &key, &t->values))
    {
        extractDVdouble(&t->values, key, &offset, &average_temperature_1h_c_);
        t->addMoreExplanation(offset, " average temperature 1h (%f C))", average_temperature_1h_c_);
    }

    if (findKey(MeasurementType::Unknown, ValueInformation::ExternalTemperature, 2, 0, &key, &t->values))
    {
        extractDVdouble(&t->values, key, &offset, &average_temperature_24h_c_);
        t->addMoreExplanation(offset, " average temperature 24h (%f C))", average_temperature_24h_c_);
    }

    // Temporarily silly solution until the dvparser is upgraded with support for extension

    key = "02FB1A"; // 1A = 0001 1010 = First extension vif code Relative Humidity 10^-1
    if (hasKey(&t->values, key))
    {
        double tmp;
        extractDVdouble(&t->values, key, &offset, &tmp, false);
        current_relative_humidity_rh_ = tmp / (double)10.0;
        t->addMoreExplanation(offset, " current relative humidity (%f RH)", current_relative_humidity_rh_);
    }
    key = "42FB1A"; // 1A = 0001 1010 = First extension vif code Relative Humidity 10^-1
    if (hasKey(&t->values, key))
    {
        double tmp;
        extractDVdouble(&t->values, key, &offset, &tmp, false);
        average_relative_humidity_1h_rh_ = tmp / (double)10.0;
        t->addMoreExplanation(offset, " average relative humidity 1h (%f RH)", average_relative_humidity_1h_rh_);
    }
    key = "8201FB1A"; // 1A = 0001 1010 = First extension vif code Relative Humidity 10^-1
    if (hasKey(&t->values, key))
    {
        double tmp;
        extractDVdouble(&t->values, key, &offset, &tmp, false);
        average_relative_humidity_24h_rh_ = tmp / (double)10.0;
        t->addMoreExplanation(offset, " average relative humidity 24h (%f RH)", average_relative_humidity_24h_rh_);
    }
}
