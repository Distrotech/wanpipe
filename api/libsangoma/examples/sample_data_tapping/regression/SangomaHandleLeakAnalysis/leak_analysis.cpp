#include <iostream>
#include "libsangoma.h"

///////////////////////////////////////////////////////////////////////////////////////////////
/// \fn         main()
/// \brief      Main entry point for the application.
/// \author     Jeff Barbieri
/// \date       11/6/2012
///////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    while(true)
    {
        // OPEN A SANGOMA PORT. 
        const unsigned int SANGOMA_PORT_INDEX = 1;
        sng_fd_t sangoma_port = sangoma_open_driver_ctrl(SANGOMA_PORT_INDEX);
        bool sangoma_port_opened = (sangoma_port != INVALID_HANDLE_VALUE);
        if (!sangoma_port_opened)
        {
            std::cout << "Could not open the Sangoma port. " << std::endl;
            system("pause");
            return;
        }
        // STOP THE SANGOMA PORT. 
        // Initialize the result of the stop command so that errors can be detected. 
        port_management_struct_t sangoma_driver_port_stop_result_details;
        memset(&sangoma_driver_port_stop_result_details, 0x00, sizeof(sangoma_driver_port_stop_result_details));
        sangoma_driver_port_stop_result_details.operation_status = SANG_STATUS_GENERAL_ERROR;
    
        // Issue the command to stop the port. 
        int sangoma_driver_port_stop_result = sangoma_driver_port_stop(
            sangoma_port,
            &sangoma_driver_port_stop_result_details,
            SANGOMA_PORT_INDEX);

        // GET THE DEFAULT SANGOMA PORT CONFIGURATION. 
        // Initialize port configuration. 
        port_cfg_t sangoma_port_configuration;
        memset(&sangoma_port_configuration, 0x00, sizeof(sangoma_port_configuration));

        // Issue the command to get the default port configuration. 
        int sangoma_driver_port_get_config_result = sangoma_driver_port_get_config(
            sangoma_port, 
            &sangoma_port_configuration,
            SANGOMA_PORT_INDEX);
    
        // Check if the configuration was retrieved. 
        const int SANGOMA_DRIVER_PORT_GET_CONFIG_RESULT_SUCCESS = 0;
        bool port_configuration_retrieved = 
            (SANGOMA_DRIVER_PORT_GET_CONFIG_RESULT_SUCCESS == sangoma_driver_port_get_config_result);
        if (!port_configuration_retrieved)
        {
            std::cout << "Could not retrieve the current port configuration. " << std::endl;
            system("pause");
            return;
        }

        // SET THE SANGOMA PORT CONFIGURATION. 
        // Issue the command to set the port configuration. 
        int sangoma_driver_port_set_config_result = sangoma_driver_port_set_config(
            sangoma_port, 
            &sangoma_port_configuration,
            SANGOMA_PORT_INDEX);
    
        // Check if the configuration was set. 
        const int SANGOMA_DRIVER_PORT_SET_CONFIG_RESULT_SUCCESS = 0;
        bool port_configuration_set = 
            (SANGOMA_DRIVER_PORT_SET_CONFIG_RESULT_SUCCESS == sangoma_driver_port_set_config_result);
        if (!port_configuration_set)
        {
            std::cout << "Could not set the port configuration. " << std::endl;
            system("pause");
            return;
        }

        // START THE SANGOMA PORT. 
        // Initialize the result of the start command so that errors can be detected. 
        port_management_struct_t sangoma_driver_port_start_result_details;
        memset(&sangoma_driver_port_start_result_details, 0x00, sizeof(sangoma_driver_port_start_result_details));
        sangoma_driver_port_start_result_details.operation_status = SANG_STATUS_GENERAL_ERROR;
    
        // Issue the command to start the port. 
        int sangoma_driver_port_start_result = sangoma_driver_port_start(
            sangoma_port,
            &sangoma_driver_port_start_result_details,
            SANGOMA_PORT_INDEX);
    
        // Check if the port was started successfully. 
        const int SANGOMA_DRIVER_PORT_START_RESULT_SUCCESS = 0;
        bool sagoma_port_started = 
            (SANGOMA_DRIVER_PORT_START_RESULT_SUCCESS == sangoma_driver_port_start_result) &&
            (SANG_STATUS_SUCCESS == sangoma_driver_port_start_result_details.operation_status);
        if (!sagoma_port_started)
        {
            std::cout << "Could not start the Sangoma port. " << std::endl;
            system("pause");
            return;
        }
    
        // STOP THE SANGOMA PORT. 
        // Initialize the result of the stop command so that errors can be detected. 
        //port_management_struct_t sangoma_driver_port_stop_result_details;
        memset(&sangoma_driver_port_stop_result_details, 0x00, sizeof(sangoma_driver_port_stop_result_details));
        sangoma_driver_port_stop_result_details.operation_status = SANG_STATUS_GENERAL_ERROR;
    
        // Issue the command to stop the port. 
        sangoma_driver_port_stop_result = sangoma_driver_port_stop(
            sangoma_port,
            &sangoma_driver_port_stop_result_details,
            SANGOMA_PORT_INDEX);
        
        // Check if the port was stopped successfully. 
        const int SANGOMA_DRIVER_PORT_STOP_RESULT_SUCCESS = 0;
        bool sangoma_driver_port_stop_result_details_indicate_success = 
            (SANG_STATUS_SUCCESS == sangoma_driver_port_stop_result_details.operation_status) ||
            (SANG_STATUS_CAN_NOT_STOP_DEVICE_WHEN_ALREADY_STOPPED == sangoma_driver_port_stop_result_details.operation_status);
        bool sangoma_port_stopped = 
            (SANGOMA_DRIVER_PORT_STOP_RESULT_SUCCESS == sangoma_driver_port_stop_result) &&
            sangoma_driver_port_stop_result_details_indicate_success;
        if (!sangoma_port_stopped)
        {
            std::cout << "Could not stop the Sangoma port. " << std::endl;
            system("pause");
            return;
        }

        // CLOSE THE SANGOMA PORT. 
        sangoma_close(&sangoma_port);

        // INDICATE THAT THE TEST IS COMPLETE. 
        std::cout << "Test complete. " << std::endl;
        //system("pause");
#ifdef linux
		sleep(1);
#else
		Sleep(1000);
#endif
		
    } // end while
    
    return;
}
