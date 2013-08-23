/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef DBUS_DYNAMIC_LOADING_DEFINITIONS_H_
#define DBUS_DYNAMIC_LOADING_DEFINITIONS_H_

#include <cstring>
#include <string>

#include <CommonAPI/CommonAPI.h>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

#include <CommonAPI/utils.h>

#include <commonapi/tests/TestInterfaceProxy.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>


const char testServiceAddress[] = "local:commonapi.tests.TestInterface:commonapi.tests.TestInterface";

const std::string COMMONAPI_CONFIG_SUFFIX = ".conf";
const std::string COMMONAPI_ENVIRONMENT_BINDING_PATH = "COMMONAPI_BINDING_PATH";

const std::string currentBinaryFileFQN = CommonAPI::getCurrentBinaryFileFQN();
const std::string currentWorkingDirectory = currentBinaryFileFQN.substr(0, currentBinaryFileFQN.find_last_of("/") + 1);

const std::string firstAliasDefinition = "alias=MyFirstAlias\n";
const std::string secondAliasDefinition = "alias=MySecondAlias\n";
const std::string combinedAliasDefinition = "alias=MyFirstAlias:MySecondAlias\n";
const std::string libraryDBusPathDefinition = "libpath=" + currentWorkingDirectory + "libCommonAPI-DBus.so\n";
const std::string libraryFakePathDefinition = "libpath=" + currentWorkingDirectory + "libCommonAPI-Fake.so\n";
const std::string generatedDBusPathDefinition = "genpath=" + currentWorkingDirectory + "libSomeOtherNameForGeneratedDBus.so\n";
const std::string __garbageString =
            ""
            "{not#a$valid/binding+PATH}\n"
            "{}\n"
            "   98t3hpgjvqpvnü0 t4b+    qßk4 kv+üg4krgv+ß4krgv+ßkr   \n"
            "{binding:blub}\n"
            "{}đwqervqerverver\n"
            "{too:much:binding}\n"
            "{binding:too:much}\n"
            "jfgv2nqp3 riqpnvi39r{}\n"
            "{hbi8uolnjk:}.-,0::9p:o:{}: bjk}{ {8h.ubpu:bzu}\n"
            "\n"
            "\n"
            "\n"
            "{noBinding:/useless/path}\n"
            "{:incomplete}\n";


const std::string validForLocalDBusBinding =
            "{binding:DBus}\n" +
            combinedAliasDefinition +
            libraryDBusPathDefinition +
            "default\n" +
            "{binding:Fake}\n" +
            "alias=DBus\n";

const std::string validForMultiplyDefinedDBusBinding =
            "{binding:DBus}\n" +
            libraryDBusPathDefinition +
            generatedDBusPathDefinition +
            firstAliasDefinition +
            "\n" +
            "{binding:DBus}\n" +
            secondAliasDefinition +
            "libpath=/useless/path";

const std::string mixedValidityValuesAndBindings =
            "{binding: InvalidBinding}\n" +
            firstAliasDefinition +
            __garbageString +
            libraryDBusPathDefinition +
            "\n" +
            "{binding:BrokenPathBinding}\n" +
            "libpath=/some/path/to/nowhere\n" +
            "\n" +
            validForMultiplyDefinedDBusBinding +
            "\n"
            "{binding: }\n" +
            __garbageString +
            "alias=emptyBindingAlias\n"
            "libpath=/another/path/to/nowhere\n" +
            __garbageString;

const std::string noValidityValuesAndBindings =
            __garbageString +
            "{binding: InvalidBinding}\n" +
            firstAliasDefinition +
            __garbageString +
            libraryDBusPathDefinition +
            "{binding: }\n" +
            __garbageString +
            "alias=emptyBindingAlias\n"
            "libpath=/some/path/to/nowhere\n" +
            "default\n" +
            __garbageString +
            "{binding:DBus}\n" +
            "genpath=" + currentWorkingDirectory + "libNonsense.so\n";


#endif /* DBUS_DYNAMIC_LOADING_DEFINITIONS_H_ */
