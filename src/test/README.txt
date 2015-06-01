This readme contains information for executing the CommonAPI-D-Bus unit tests.

Before executing the tests it has to be secured, that the LD_LIBRARY_PATH contains the folder containing the dbus libraries.

Information for DBusAddressTranslatorTest:
The environment variable COMMONAPI_DBUS_DEFAULT_CONFIG must be set. The variable has to contain the absolute path to the commonapi-dbus.ini file contained in the folder src/test of the CommonAPI-D-Bus binding.
e.g. export COMMONAPI_DBUS_DEFAULT_CONFIG=/home/<user>/git/ascgit017.CommonAPI-D-Bus/src/test/commonapi-dbus.ini

Furthermore the environment variable TEST_COMMONAPI_DBUS_ADDRESS_TRANSLATOR_FAKE_LEGACY_SERVICE_FOLDER must be set. The variable has to contain the absolute path of the folder, containing the FakeLegacyService python files.
e.g. export TEST_COMMONAPI_DBUS_ADDRESS_TRANSLATOR_FAKE_LEGACY_SERVICE_FOLDER=/home/<user>/git/ascgit017.CommonAPI-D-Bus/src/test/fakeLegacyService


