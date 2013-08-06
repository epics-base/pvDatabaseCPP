/*ExamplePVADoubleArrayGetMain.cpp */
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
#include <pv/examplePVADoubleArrayGet.h>
#include <pv/traceRecord.h>
#include <pv/channelProviderLocal.h>
#include <pv/serverContext.h>
#include <pv/clientFactory.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

static StandardPVFieldPtr standardPVField = getStandardPVField();

int main(int argc,char *argv[])
{
    PVDatabasePtr master = PVDatabase::getMaster();
    ClientFactory::start();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();
    PVRecordPtr pvRecord;
    bool result(false);
    String recordName;
    PVStructurePtr pvStructure = standardPVField->scalarArray(
        pvDouble,"alarm,timeStamp");
    recordName = "doubleArray";
    pvRecord = PVRecord::create(recordName,pvStructure);
    result = master->addRecord(pvRecord);
    if(!result) cout<< "record " << recordName << " not added" << endl;
    ServerContext::shared_pointer serverContext = startPVAServer(PVACCESS_ALL_PROVIDERS,0,true,true);
    recordName = "examplePVADoubleArrayGet";
    if(argc>1) recordName = argv[1];
    String providerName("local");
    if(argc>2) providerName = argv[2];
    String channelName("doubleArray");
    if(argc>3) channelName = argv[3];
    pvRecord = ExamplePVADoubleArrayGet::create(
        recordName,providerName,channelName);
    if(pvRecord!=NULL) {
        result = master->addRecord(pvRecord);
        cout << "result of addRecord " << recordName << " " << result << endl;
    } else {
        cout << "ExamplePVADoubleArrayGet::create failed" << endl;
    }
    string str;
    while(true) {
        cout << "Type exit to stop: \n";
        getline(cin,str);
        if(str.compare("exit")==0) break;

    }
    serverContext->shutdown();
    epicsThreadSleep(1.0);
    serverContext->destroy();
    ClientFactory::stop();
    channelProvider->destroy();
    return 0;
}

