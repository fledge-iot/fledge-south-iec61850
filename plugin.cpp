#include <iec61850.h>
#include <plugin_api.h>
#include <stdio.h>
#include <string>
#include <logger.h>
#include <config_category.h>
#include <rapidjson/document.h>
#include <version.h>
#include <iec61850_model.h>

typedef void (*INGEST_CB)(void *, Reading);

using namespace std;

#define PLUGIN_NAME "iec61850"

/**
 * Default configuration
 */

const char *default_config = QUOTE ({
                                        "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "default" : PLUGIN_NAME,
        "readonly" : "true"
    },

                                        "asset" : {
        "description" : "Asset name",
        "type" : "string",
        "default" : "iec61850",
        "displayName" : "Asset Name",
        "order" : "1",
        "mandatory" : "true"
    },

                                        "ip" : {
        "description" : "IP of the Server",
        "type" : "string",
        "default" : "127.0.0.1",
        "displayName" : "61850 Server IP",
        "order" : "2"
    },

                                        "port" : {
        "description" : "Port number of the 61850 server",
        "type" : "integer",
        "default" : "8102",
        "displayName" : "61850 Server port"

    },

                                        "IED Model" : {
        "description" : "Name of the 61850 IED model",
        "type" : "string", //IedModel
        "default" : "testmodel",
        "displayName" : "61850 Server IedModel"

    },
                                        "Logical Device" : {
        "description" : "Logical device of the 61850 server",
        "type" : "string", //LogicalDevice
        "default" : "SENSORS",
        "displayName" : "61850 Server logical device"

    },

                                        "Logical Node" : {
        "description" : "Logical node of the 61850 server",
        "type" : "string", //LogicalNode
        "default" : "[TTMP1]",
        "displayName" : "61850 Server logical node"

    },

                                        "CDC" : {
        "description" : "CDC name of the 61850 server",
        "type" : "string", //CDC_SAV
        "default" : "TmpSv",
        "displayName" : "61850 Server CDC_SAV"

    }


                                    });

/**
 * The 104 plugin interface
 */
extern "C" {
static PLUGIN_INFORMATION info = {
        PLUGIN_NAME,              // Name
        VERSION,                  // Version
        SP_ASYNC,          // Flags
        PLUGIN_TYPE_SOUTH,        // Type
        "1.0.0",                  // Interface version
        default_config          // Default configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info() {
    Logger::getLogger()->info("61850 Config is %s", info.config);
    return &info;
}

PLUGIN_HANDLE plugin_init(ConfigCategory *config) {
    IEC61850 *iec61850;
    Logger::getLogger()->info("Initializing the plugin");

    string ip = "127.0.0.1";
    string model="testmodel";
    string logicalNode;
    string logicalDevice;
    string cdc;
    uint16_t port = 8102;


    if (config->itemExists("ip")){
        ip = config->getValue("ip");
    }

    if (config->itemExists("port")){
        port = static_cast<uint16_t>(stoi(config->getValue("port")));
        char str[80];
        sprintf(str, "%u", port);
    }

    if (config->itemExists("IED Model")) {
        model = (config->getValue("IED Model"));
    }

    if (config->itemExists("Logical Device")) {
        logicalDevice = config->getValue("Logical Device");
    }

    if (config->itemExists("Logical Node")) {
        logicalNode = config->getValue("Logical Node");
    }

    if (config->itemExists("CDC")) {
        cdc = config->getValue("CDC");
    }

    iec61850 = new IEC61850(ip.c_str(), port, model, logicalNode, logicalDevice, cdc);

    if (config->itemExists("asset")) {
        iec61850->setAssetName(config->getValue("asset"));
    } else {
        iec61850->setAssetName("iec61850");
    }

    return (PLUGIN_HANDLE) iec61850;
}

/**
 * Start the Async handling for the plugin
 */
void plugin_start(PLUGIN_HANDLE *handle) {
    if (!handle)
        return;
    Logger::getLogger()->info("Starting the plugin");
    IEC61850 *iec61850 = (IEC61850 *) handle;
    iec61850->start();

}

/**
 * Register ingest callback
 */
void plugin_register_ingest(PLUGIN_HANDLE *handle, INGEST_CB cb, void *data) {
    if (!handle)
        throw new exception();

    IEC61850 *iec61850 = (IEC61850 *) handle;
    iec61850->registerIngest(data, cb);
}

/**
 * Poll for a plugin reading
 */
Reading plugin_poll(PLUGIN_HANDLE *handle) {

    throw runtime_error("IEC_104 is an async plugin, poll should not be called");
}

/**
 * Reconfigure the plugin
 *
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, string &newConfig) {

    Logger::getLogger()->warn("Reconfigure");

    ConfigCategory config("new", newConfig);
    auto *iec61850 = (IEC61850 *) *handle;

    string ip;
    string model;
    string logicalNode;
    string logicalDevice;
    string cdc;
    uint16_t port;

    iec61850->stop();

    if (config.itemExists("ip")){
        ip = config.getValue("ip");
        iec61850->setIp(ip.c_str());
    }

    if (config.itemExists("port")){
        port = static_cast<uint16_t>(stoi(config.getValue("port")));
        char str[80];
        sprintf(str, "%u", port);
        iec61850->setPort(port);
    }

    if (config.itemExists("IED Model")) {
        model = (config.getValue("IED Model"));
        iec61850->setModel(model);
    }

    if (config.itemExists("Logical Device")) {
        logicalDevice = config.getValue("Logical Device");
        iec61850->setLogicalDevice(logicalDevice);
    }

    if (config.itemExists("Logical Node")) {
        logicalNode = config.getValue("Logical Node");
        iec61850->setLogicalNode(logicalNode);
    }

    if (config.itemExists("CDC")) {
        cdc = config.getValue("CDC");
        iec61850->setCdc(cdc);
    }

    if (config.itemExists("asset")) {
        iec61850->setAssetName(config.getValue("asset"));
    } else {
        iec61850->setAssetName("iec61850");
    }

    iec61850->start();

}

/**
 * Shutdown the plugin
*/
void plugin_shutdown(PLUGIN_HANDLE *handle) {
    auto *iec61850 = (IEC61850 *) handle;

    iec61850->stop();
    delete iec61850;
}


}
