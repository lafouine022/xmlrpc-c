#include <iostream>
#include <stdexcept>

#include "xmlrpcType.hpp"
#include "xmlrpcMethod.hpp"
#include "proxyClass.hpp"
#include "systemProxy.hpp"

using namespace std;


/*----------------------------------------------------------------------------
   Command line
-----------------------------------------------------------------------------*/

class cmdlineInfo {
public:
    string serverUrl;
    string methodPrefix;
    string localClass;

    cmdlineInfo(int           const argc,
                const char ** const argv);

private:
    cmdlineInfo();
};



cmdlineInfo::cmdlineInfo(int           const argc,
                         const char ** const argv) {

    if (argc-1 != 3) {
        cerr << "There are 3 arguments: server URL, "
             << "prefix for the methods to include (null to include all), "
             << "and name to give the generated proxy class.  "
             << "You specified " << argc-1 << " arguments."
             << endl
             << "Example:  "
             << "xmlrpc_cpp_proxy http://localhost/RPC2 system systemProxy"
             << endl;
        exit(1);
    }
    this->serverUrl    = string(argv[1]);
    this->methodPrefix = string(argv[2]);
    this->localClass   = string(argv[3]);
}



static proxyClass
getClassInfo(string const& serverUrl,
             string const& classPrefix,
             string const& className) {
/*----------------------------------------------------------------------------
  Connect to a remote server and extract the information we'll need to
  build a proxy class.
-----------------------------------------------------------------------------*/
    proxyClass theClass(className);

    systemProxy system;

    xmlrpc_c::value_array methods(system.listMethods(serverUrl));

    unsigned int arraySize = methods.size();
    
    for (size_t i = 0; i < arraySize; ++i) {

        // Break the method name into two pieces.
        xmlrpc_c::value_string val = (methods.vectorValueValue())[i];
        string const methodName(static_cast<string>(val));
        size_t const lastDot(methodName.rfind('.'));

        string methodPrefix;
        string functionName;

        if (lastDot == string::npos) {
            methodPrefix = "";
            functionName = methodName;
        } else {
            methodPrefix = string(methodName, 0, lastDot);
            functionName = string(methodName, lastDot + 1);
        }

        if (methodPrefix == classPrefix) {
            // It's a method User cares about

            string const help(system.methodHelp(serverUrl, methodName));
            xmlrpc_c::value const signatureList(
                system.methodSignature(serverUrl, methodName));

            if (signatureList.type() != xmlrpc_c::value::TYPE_ARRAY) {
                // It must be the string "undef", meaning the server
                // won't tell us any signatures.
                cerr << "Skipping method " << methodName << " "
                     << "because server does not report any signatures "
                     << "for it (via system.methodSignature method)"
                     << endl;
            } else {
                // Add this function to our class information.
                xmlrpcMethod const method(
                    functionName,
                    methodName,
                    help,
                    xmlrpc_c::value_array(signatureList));
                theClass.addFunction(method);
            }
        }
    }
    return theClass;
}



static void
printHeader(ostream          & out,
            proxyClass const& classInfo) {
/*----------------------------------------------------------------------------
  Print a complete header for the specified class.
-----------------------------------------------------------------------------*/
    string const className(classInfo.className());

    try {
        out << "// Interface definition for " << className << " class, "
            << "an XML-RPC FOR C/C++ proxy class" << endl;
        out << "// Generated by 'xmlrpc_cpp_proxy'" << endl;
        out << endl;

        string const headerSymbol("_" + className + "_H_");

        out << "#ifndef " << headerSymbol << endl;
        out << "#define " << headerSymbol << " 1" << endl;
        out << endl;
        out << "#include <string>" << endl;
        out << "#include <xmlrpc-c/client_simple.hpp>" << endl;
        out << endl;

        classInfo.printDeclaration(cout);

        out << endl;
        out << "#endif /* " << headerSymbol << " */" << endl;
    } catch (exception const& e) {
        throw(logic_error("Failed to generate header for class " +
                          className + ".  " + e.what()));
    }
}



static void
printCppFile(ostream          & out,
             proxyClass const& classInfo) {
/*----------------------------------------------------------------------------
  Print a complete definition for the specified class.
-----------------------------------------------------------------------------*/
    string const className(classInfo.className());

    try {
        out << "// " << className << " - "
            << "an XML-RPC FOR C/C++ proxy class" << endl;
        out << "// Generated by 'xmlrpc_cpp_proxy'" << endl;
        out << endl;
        
        out << "#include \"" << className << ".h\"" << endl;
        
        classInfo.printDefinition(cout);
    } catch (xmlrpc_c::fault const& f) {
        throw(logic_error("Failed to generate definition for class " +
                          className + ".  " + f.getDescription()));
    }
}



int
main(int           const argc,
     const char ** const argv) {

    string const myName(argv[0]);

    cmdlineInfo const cmdline(argc, argv);

    int retval;

    try {
        proxyClass system(getClassInfo(cmdline.serverUrl,
                                       cmdline.methodPrefix,
                                       cmdline.localClass));
        printHeader(cout, system);
        cout << endl;
        printCppFile(cout, system);
        retval = 0;
    } catch (xmlrpc_c::fault& f) {
        cerr << myName << ": XML-RPC fault #" << f.getCode()
             << ": " << f.getDescription() << endl;
        retval = 1;
    } catch (exception const& e) {
        cerr << myName << ": " << e.what() << endl;
        retval = 1;
    } catch (...) {
        cerr << myName << ": Unknown exception" << endl;
        retval = 1;
    }

    return retval;
}