//
// Created by estelle on 28/10/2020.
//

#include <iec61850.h>
#include <zconf.h>
#include "iec61850_client.h"
#include "hal_thread.h"
#include <string>

using namespace std;

IEC61850::IEC61850(const char *ip, uint16_t port, string iedModel, std::string logicalNode, std::string logicalDevice,std::string cdc) {
    m_ip = ip;
    m_port = port;
    m_logicalnode = logicalNode;
    m_logicaldevice = logicalDevice;
    m_iedmodel = iedModel;
    m_cdc = cdc;
    m_iedconnection = nullptr;
}

//Set the IP of the 61850 server 
void IEC61850::setIp(const char *ip) {
    if (strlen(ip) > 1)
        /* Ip set to the ip given by user */
        m_ip = ip;
    else{
        /* Default IP if entry null*/
        m_ip = "127.0.0.1";
    }
}

//Set the port of the 61850 server
void IEC61850::setPort(uint16_t port) {
    Logger::getLogger()->warn("set port");
    if (port>0){
        /* port set to the port given by the user*/
        m_port = port;
    }
    else{
        /* Default port for IEC61850 */
        m_port = 8102;
    }
}

//Set the name of the asset
void IEC61850::setAssetName(const std::string &name) {
        m_asset = name;
}

//Set the name of the logical device
void IEC61850::setLogicalDevice(std::string logicaldevice_name) {
    m_logicaldevice = logicaldevice_name;
}

//Set the name of the logical node
void IEC61850::setLogicalNode(std::string logicalnode_name) {
    m_logicalnode = logicalnode_name;
}

//Set the name of the IED model
void IEC61850::setModel(string model) {
    m_iedmodel = model;
}

//Set the name of the CDC
void IEC61850::setCdc(string CDC) {
    m_cdc = CDC;
}

/* Restart the iec61850 plugin */
void IEC61850::restart() {
    Logger::getLogger()->warn("Restart ");
    /* stop the iec61850 plugin */
    stop();
    /* restart the iec61850 plugin after closing it */
    start();
}

void IEC61850::start() {

    Logger::getLogger()->warn("Plugin started");
    /* Creating the client for fledge */
    m_client = new IEC61850Client(this);
    /* The type of Data class */
    m_goto = m_iedmodel + m_logicaldevice + "/" + m_logicalnode + "." + m_cdc + "." + "instMag.f"; //instMag.f

    /* Retry if connection lost */
    while (1){
        /* Create the connection object reference */
        stop();
        m_iedconnection = IedConnection_create();
        /* Connect with the object connection reference */
        IedConnection_connect(m_iedconnection,&m_error,m_ip.c_str(),m_port);
        if (nullptr != m_iedconnection){
            while (IedConnection_getState(m_iedconnection)) {
                if (m_error == IED_ERROR_OK) {
                    /* read an analog measurement value from server */
                    MmsValue *value = IedConnection_readObject(m_iedconnection, &m_error, m_goto.c_str(), IEC61850_FC_MX);
                    /* The value should not be null */
                    if (value != nullptr) {
                        /* Test the value type */
                        switch (MmsValue_getType(value))  {
                            case (MMS_FLOAT) :
                                m_client->sendData("MMS_FLOAT", MmsValue_toFloat(value));
                                break;

                            case(MMS_BOOLEAN):
                                const char *bval;
                                MmsValue_setMmsString(value, bval);
                                m_client -> sendData("MMS_BOOLEAN", bval);
                                break;

                            case(MMS_INTEGER):
                                m_client -> sendData("MMS_INTEGER", long(MmsValue_toInt32(value)));
                                break;

                            case(MMS_VISIBLE_STRING) :
                                m_client->sendData("MMS_VISIBLE_STRING", MmsValue_toString(value));
                                break;

                            case(MMS_UNSIGNED):
                                m_client->sendData("MMS_VISIBLE_STRING", long(MmsValue_toUint32(value)));
                                break;

                            case (MMS_DATA_ACCESS_ERROR) :
                                Logger::getLogger()->warn("MMS access error, please reconfigure");
                                sleep(1);
                                break;
                            default :
                                break;

                        }
                        MmsValue_delete(value);

                    } else {
                        Logger::getLogger()->warn("Value error, please reconfigure");
                        break;
                    }
                }else{
                    Logger::getLogger()->warn("Connection error, please reconfigure");
                    break;
                }
                //sleep(1);
            }
        }
        else{
            Logger::getLogger()->warn("Connection error");
        }
        sleep(1);
    }

}

void IEC61850::stop() {
    if (m_iedconnection != nullptr && IedConnection_getState(m_iedconnection)){
        /* Close the connection */
        IedConnection_close(m_iedconnection);
        /* Destroy the connection instance after closing it */
        IedConnection_destroy(m_iedconnection);
    }
}

void IEC61850::ingest(std::vector<Datapoint *> points) {
    /* Creating the name of the type of data */
    string asset = points[0]->getName();
    /* Callback function used after receiving data */
    (*m_ingest)(m_data, Reading(asset, points));

}




IEC61850::~IEC61850()=default;




