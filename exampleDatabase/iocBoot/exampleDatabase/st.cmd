< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/exampleDatabase.dbd")
exampleDatabase_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/dbScalar.db","name=double00,type=ao")
dbLoadRecords("db/dbScalar.db","name=double01,type=ao")
dbLoadRecords("db/dbScalar.db","name=double02,type=ao")
dbLoadRecords("db/dbScalar.db","name=double03,type=ao")
dbLoadRecords("db/dbScalar.db","name=double04,type=ao")
dbLoadRecords("db/dbScalar.db","name=double05,type=ao")
dbLoadRecords("db/dbStringArray.db","name=stringArray01")
dbLoadRecords("db/dbEnum.db","name=enum01")
dbLoadRecords("db/dbCounter.db","name=counter01");

cd ${TOP}/iocBoot/${IOC}
iocInit()
dbl
epicsThreadSleep(2.0)
exampleDatabase
exampleMonitorPlugin
startPVAServer
pvdbl
