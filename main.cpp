#include <vector>

#include "stlink.h"
#include "log.h"

int main(int argc, char **argv)
{

    log_init();
    debug_level = log_levels::LOG_LVL_DEBUG_IO;

    LOG_INFO("App started");

    struct hl_interface_param_s param;
    param.transport = hl_transports::HL_TRANSPORT_SWD;

    std::vector<uint16_t> pids{STLINK_V2_PID, STLINK_V2_1_PID, STLINK_V2_1_NO_MSD_PID,
                               STLINK_V3_USBLOADER_PID, STLINK_V3E_PID, STLINK_V3S_PID, STLINK_V3_2VCP_PID, STLINK_V3E_NO_MSD_PID};

    for (std::size_t i = 0; (i < pids.size()) && (i < HLA_MAX_USB_IDS); ++i)
    {
        param.vid[i] = STLINK_VID;
        param.pid[i] = pids[i];
    }

    param.use_stlink_tcp = false;

    void *fd;

    int res = stlink_usb_layout_api.open(&param, &fd);
    res = stlink_usb_layout_api.close(&fd);
}