/// @brief OPC UA Server main.
/// @license GNU LGPL
///
/// Distributed under the GNU LGPL License
/// (See accompanying file LICENSE or copy at
/// http://www.gnu.org/licenses/lgpl.html)
///
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <thread>         
#include <chrono>   

#include <opc/ua/node.h>
#include <opc/ua/subscription.h>
#include <opc/ua/server/server.h>

using namespace OpcUa;

std::vector<Node> manyObjects(1000);

class SubClient : public SubscriptionHandler
{
  void DataChange(uint32_t handle, const Node& node, const Variant& val, AttributeId attr) override
  {
    std::cout << "Received DataChange event for Node " << node << std::endl;
  }
};

inline uint8_t operator|(VariableAccessLevel x, VariableAccessLevel y)
{
    return (uint8_t) ((uint8_t)x | (uint8_t)y);
}

void addMany(Node &objects,int nrOfObjects,uint32_t serverNamespace)
{
    NodeId nid(100, serverNamespace);
    QualifiedName qn("ManyObjects", serverNamespace);
    Node newobject = objects.AddObject(nid, qn);

    DataValue writeIt(VariableAccessLevel::CurrentRead|VariableAccessLevel::CurrentWrite);

    for(int i=0; i<nrOfObjects;i++) {
        std::string name("var" + std::to_string(i+1));
        Node myprop = newobject.AddVariable(serverNamespace, name, Variant(i+1));
        myprop.SetAttribute(AttributeId::AccessLevel,writeIt);
        myprop.SetAttribute(AttributeId::UserAccessLevel,writeIt);
        manyObjects.at(i) = myprop;
    }
}

void RunServer(int nrOfObjects,int wait,int debug)
{
  //First setup our server
  bool dbg = false;
  if(debug > 1) dbg = true;
  OpcUa::UaServer server(dbg);
  server.SetEndpoint("opc.tcp://elbe.acc.bessy.de:4841/freeopcua/server");
  server.SetServerURI("urn://exampleserver.freeopcua.github.io");
  server.Start();
  
  //then register our server namespace and get its index in server
  uint32_t idx = server.RegisterNamespace("http://examples.freeopcua.github.io");
  
  //Create our address space using different methods
  Node objects = server.GetObjectsNode();
  
  //Add a custom object with specific nodeid
  NodeId nid(99, idx);
  QualifiedName qn("NewObject", idx);
  Node newobject = objects.AddObject(nid, qn);

  //Add a variable and a property with auto-generated nodeid to our custom object
  Node myStrVar = newobject.AddVariable(idx, "MyStringVar", Variant(std::string("empty")));
  Node myvar = newobject.AddVariable(idx, "MyVariable", Variant(8));
  Node myprop = newobject.AddVariable(idx, "MyProperty", Variant(8.8));

  DataValue writeIt(VariableAccessLevel::CurrentRead|VariableAccessLevel::CurrentWrite);
  myprop.SetAttribute(AttributeId::AccessLevel,writeIt);
  myprop.SetAttribute(AttributeId::UserAccessLevel,writeIt);

  bool mBool = true;
  Node myBool = newobject.AddVariable(idx, "MyBool", Variant(mBool));
  myBool.SetAttribute(AttributeId::AccessLevel,writeIt);
  myBool.SetAttribute(AttributeId::UserAccessLevel,writeIt);

  std::vector<int> arrVal = {1,2,3,4,5};
  Node myArrVar = newobject.AddVariable(idx, "MyArrayVar", Variant(Variant(arrVal)));

  //browse root node on server side
  Node root = server.GetRootNode();
  std::cout << "Root node is: " << root << std::endl;
  std::cout << "Childs are: " << std::endl;
  for (Node node : root.GetChildren())
  {
    std::cout << "    " << node << std::endl;
  }

  

  //Uncomment following to subscribe to datachange events inside server
  /*
  SubClient clt;
  std::unique_ptr<Subscription> sub = server.CreateSubscription(100, clt);
  sub->SubscribeDataChange(myvar);
  */

  addMany(objects,nrOfObjects,idx);

  //Now write values to address space and send events so clients can have some fun
  uint32_t counter = 0;
  char strValChar[20];

  myvar.SetValue(Variant(counter)); //will change value and trigger datachange event
  //Create event
  server.EnableEventNotification();
  Event ev(ObjectId::BaseEventType); //you should create your own type
  ev.Severity = 2;
  ev.SourceNode = ObjectId::Server;
  ev.SourceName = "Event from FreeOpcUA";
  ev.Time = DateTime::Current();


  std::cout << "Ctrl-C to exit" << std::endl;
  for (;;)
  {
    myvar.SetValue(Variant(++counter)); //will change value and trigger datachange event

    sprintf(strValChar,"event: %3d",counter);
    myStrVar.SetValue(std::string(strValChar));

    for (idx=0;idx<arrVal.size();idx++) {
        int v = arrVal[idx];
        arrVal[idx] = v+1;
    }
    myArrVar.SetValue(Variant(arrVal));

    if(mBool==true) {
        myBool.SetValue(Variant(mBool));
        mBool=false;
    }
    else {
        myBool.SetValue(Variant(mBool));
        mBool=true;
    }

    Variant nodeVal;
    int     val=0;
    Node nd;
    
    for(int i=0; i<nrOfObjects; i++) {
        nd = manyObjects.at(i);
        nodeVal = nd.GetValue();
        val = nodeVal.As<int>();
        val++;
        nd.SetValue(Variant(val));
    }

/*    std::stringstream ss;
    ss << "This is event number: " << counter;
    ev.Message = LocalizedText(ss.str());
    server.TriggerEvent(ev);
*/
    std::this_thread::sleep_for(std::chrono::milliseconds(wait));
  }

  server.Stop();
}

const std::string &writeEpicsDbFile(std::string &retStr,const std::string &recName,const std::string &link) 
{
    std::stringstream ret;
    ret <<   "record(ai,"<< recName <<") {\n"<<
             "  field(DESC,"<<"\""<<recName<<"\""<<")\n"<<
             "  field(SCAN,"<<"\""<<"I/O Intr"<<"\""<<")\n"<<
             "  field(PINI,YES)\n"<<
             "  field(TSE, -2)\n"<<
             "  field(DTYP,OPCUA)\n"<<
             "  field(DISS,INVALID)\n"<<
             "  field(INP,"<<"\""<< link <<"\""<<")\n"<<
             "}\n";
    retStr = ret.str();
    return retStr;
}    

int main(int argc, char** argv)
{
    int wait = 2000;
    int nrOfObjects = 1000;
    int verbose = 0;
    int epics = 0;
    int c;
    const char help[] = "testServer [OPTIONS]\n"
    "-h:   show help\n"
    "-n N: number of opcua items for ManyObjects (1000)\n"
    "-t N: Update time (1000ms)\n"
    "-e:   create testServer.db file for ManyObjects\n"
    "Test variables:\n"
    "  NewObject.MyStringVar\n"
    "  NewObject.MyVariable\n"
    "  NewObject.MyProperty\n"
    "  NewObject.MyArrayVar\n"
    "  NewObject.MyBool\n"
    "  ManyObjects.var1 ... ManyObjects.varN\n";
    
    opterr = 0;


    while ((c = getopt (argc, argv, "hev:t:n:")) != -1)
    switch (c)
    {
        case 'n':
            nrOfObjects = atoi(optarg);
            break;
        case 't':
            wait = atoi(optarg);
            break;
        case 'v':
            verbose = atoi(optarg);
            break;
        case 'e':
            epics=1;
            break;
        case 'h':
            printf("%s",help);
            exit(0);
            break;
    }
    std::cout << "Create ManyObjects:var1 to ManyObjects:var"<<nrOfObjects<<std::endl;
    std::cout << "Update (ms): "<<wait<<std::endl;
    if(epics) {
        std::stringstream sRec;
        std::stringstream sLink;
        std::string recordStr;
        printf("writeEpicsDbFile\n");
        std::ofstream myfile;
        myfile.open ("testServer.db");
        for(int i=0;i<nrOfObjects;i++) {
            sRec.str( std::string() );
            sRec.clear();
            sRec << "ManyObjects:var"<<i;

            sLink.str( std::string() );
            sLink.clear();
            sLink << "2:ManyObjects.var"<<i;

            recordStr.clear();
            myfile << writeEpicsDbFile(recordStr,sRec.str(),sLink.str());
        }
        
        myfile.close();
    }
    try
    {
        RunServer(nrOfObjects,wait,verbose);
    }
    catch (const std::exception& exc)
    {
        std::cout << "Catch:" <<exc.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown error." << std::endl;
    }

    return 0;
}

