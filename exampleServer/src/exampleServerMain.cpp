/*ExampleServerMain.cpp */
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
#include <pv/exampleServer.h>
#include <pv/traceRecord.h>
#include <pv/channelProviderLocal.h>
#include <pv/serverContext.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace epics::exampleServer;

int main(int argc,char *argv[])
{
    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();
    PVRecordPtr pvRecord;
    bool result(false);
    String recordName;
    recordName = "exampleServer";
    pvRecord = ExampleServer::create(recordName);
    result = master->addRecord(pvRecord);
    cout << "result of addRecord " << recordName << " " << result << endl;
    recordName = "traceRecordPGRPC";
    pvRecord = TraceRecord::create(recordName);
    result = master->addRecord(pvRecord);
    if(!result) cout<< "record " << recordName << " not added" << endl;
    pvRecord.reset();
    ServerContext::shared_pointer pvaServer = 
        startPVAServer(PVACCESS_ALL_PROVIDERS,0,true,true);
    cout << "exampleServer\n";
    string str;
    while(true) {
        cout << "Type exit to stop: \n";
        getline(cin,str);
        if(str.compare("exit")==0) break;

    }
    pvaServer->shutdown();
    epicsThreadSleep(1.0);
    pvaServer->destroy();
    channelProvider->destroy();
    return 0;
}

