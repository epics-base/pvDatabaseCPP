/*ExampleCounterMain.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/exampleCounter.h>
#include <pv/channelProviderLocal.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

int main(int argc,char *argv[])
{
    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();
    String recordName("exampleCounter");
    PVRecordPtr pvRecord = ExampleCounter::create(recordName);
    bool result = master->addRecord(pvRecord);
    cout << "result of addRecord " << recordName << " " << result << endl;
    pvRecord.reset();
    cout << "exampleServer\n";
    string str;
    while(true) {
        cout << "Type exit to stop: \n";
        getline(cin,str);
        if(str.compare("exit")==0) break;

    }
    return 0;
}

