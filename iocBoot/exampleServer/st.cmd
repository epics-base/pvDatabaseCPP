< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/exampleServer.dbd")
exampleServer_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/dbScalar.db","name=double01,type=ao")
dbLoadRecords("db/dbStringArray.db","name=stringArray01")
dbLoadRecords("db/dbEnum.db","name=enum01")
dbLoadRecords("db/dbCounter.db","name=counter01");

cd ${TOP}/iocBoot/${IOC}
iocInit()
epicsThreadSleep(2.0)
casr
exampleServerCreateRecords
startPVAServer
