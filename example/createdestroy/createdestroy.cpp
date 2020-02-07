/******************************************************************************
* This is modeled after a test program created by Bertrand Bauvir from the ITER Organization
******************************************************************************/


#include <pv/pvData.h>
#include <pv/pvDatabase.h>
#include <pv/serverContext.h>
#include <pv/channelProviderLocal.h>
#include <pva/client.h>

// Local header files

// Constants

#define DEFAULT_RECORD_NAME "examplechannel"

using std::tr1::static_pointer_cast;

// Type definition

class Record : public ::epics::pvDatabase::PVRecord
{

  public:
    std::shared_ptr<::epics::pvData::PVStructure> __pv;
    static std::shared_ptr<Record> create (std::string const & name, std::shared_ptr<::epics::pvData::PVStructure> const & pvstruct);
    Record (std::string const & name, std::shared_ptr<epics::pvData::PVStructure> const & pvstruct)
      : epics::pvDatabase::PVRecord(name, pvstruct) { __pv = pvstruct; };
    virtual void process (void);
};

std::shared_ptr<Record> Record::create (std::string const & name, std::shared_ptr<::epics::pvData::PVStructure> const & pvstruct) 
{ 

  std::shared_ptr<Record> pvrecord (new Record (name, pvstruct)); 
  
  // Need to be explicitly called .. not part of the base constructor
  if(!pvrecord->init()) pvrecord.reset();

  return pvrecord; 

}

void Record::process (void)
{
  PVRecord::process();
  std::string name = this->getRecordName();
  std::cout << this->getRecordName()
            << this->getPVStructure()
            << "\n";
}


int main (int argc, char** argv) 
{
  bool verbose = false;
  unsigned loopctr = 0;

  // Create PVA context
  ::epics::pvDatabase::PVDatabasePtr master = epics::pvDatabase::PVDatabase::getMaster();
  ::epics::pvDatabase::ChannelProviderLocalPtr channelProvider = epics::pvDatabase::getChannelProviderLocal();

  // Start PVA server
  epics::pvAccess::ServerContext::shared_pointer context = epics::pvAccess::startPVAServer(epics::pvAccess::PVACCESS_ALL_PROVIDERS, 0, true, true);


  while (true) {
      loopctr++;
      std::string name = DEFAULT_RECORD_NAME + std::to_string(loopctr);

      // Create record
      // Create record structure
      ::epics::pvData::FieldBuilderPtr builder = epics::pvData::getFieldCreate()->createFieldBuilder();
      builder->add("value", ::epics::pvData::pvULong);
      std::shared_ptr<::epics::pvData::PVStructure> pvstruct = ::epics::pvData::getPVDataCreate()->createPVStructure(builder->createStructure());
      std::shared_ptr<Record> pvrecord = Record::create(std::string(name), pvstruct);
      master->addRecord(pvrecord);
      if (verbose) pvrecord->setTraceLevel(3);
      // Start PVA (local) client
      std::tr1::shared_ptr<::pvac::ClientProvider> provider
          = std::tr1::shared_ptr<::pvac::ClientProvider>(new ::pvac::ClientProvider ("pva"));
      std::tr1::shared_ptr<::pvac::ClientChannel> channel
          = std::tr1::shared_ptr<::pvac::ClientChannel>(new ::pvac::ClientChannel (provider->connect(name)));
      builder = epics::pvData::getFieldCreate()->createFieldBuilder();
          builder->add("value", ::epics::pvData::pvULong);
          std::shared_ptr<::epics::pvData::PVStructure> c_pvstruct =
               ::epics::pvData::getPVDataCreate()->createPVStructure(builder->createStructure());
      unsigned valuectr = loopctr;
      for (int ind=0; ind<10; ind++) channel->put().set("value",valuectr++).exec();
      master->removeRecord(pvrecord);
  }
  return (0);
}
