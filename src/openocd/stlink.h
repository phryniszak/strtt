#ifndef PH_STLINK_H
#define PH_STLINK_H

#include "helper_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define STLINK_V1_PID (0x3744)
#define STLINK_V2_PID (0x3748)
#define STLINK_V2_1_PID (0x374B)
#define STLINK_V2_1_NO_MSD_PID (0x3752)
#define STLINK_V3_USBLOADER_PID (0x374D)
#define STLINK_V3E_PID (0x374E)
#define STLINK_V3S_PID (0x374F)
#define STLINK_V3_2VCP_PID (0x3753)
#define STLINK_V3E_NO_MSD_PID (0x3754)
#define STLINK_VID (0x0483)


    /** */
    extern struct hl_layout_api_s stlink_usb_layout_api;

    /*
 * TARGET_UNKNOWN = 0: we don't know anything about the target yet
 * TARGET_RUNNING = 1: the target is executing or ready to execute user code
 * TARGET_HALTED  = 2: the target is not executing code, and ready to talk to the
 * debugger. on an xscale it means that the debug handler is executing
 * TARGET_RESET   = 3: the target is being held in reset (only a temporary state,
 * not sure how this is used with all the recent changes)
 * TARGET_DEBUG_RUNNING = 4: the target is running, but it is executing code on
 * behalf of the debugger (e.g. algorithm for flashing)
 *
 * also see: target_state_name();
 */

    enum target_state
    {
        TARGET_UNKNOWN = 0,
        TARGET_RUNNING = 1,
        TARGET_HALTED = 2,
        TARGET_RESET = 3,
        TARGET_DEBUG_RUNNING = 4,
    };

    /* Values should match TPIU_SPPR_PROTOCOL_xxx */
    enum tpiu_pin_protocol
    {
        TPIU_PIN_PROTOCOL_SYNC = 0,             /**< synchronous trace output */
        TPIU_PIN_PROTOCOL_ASYNC_MANCHESTER = 1, /**< asynchronous output with Manchester coding */
        TPIU_PIN_PROTOCOL_ASYNC_UART = 2,       /**< asynchronous output with NRZ coding */
    };

    enum hl_transports
    {
        HL_TRANSPORT_UNKNOWN = 0,
        HL_TRANSPORT_SWD,
        HL_TRANSPORT_JTAG,
    };

#define HLA_MAX_USB_IDS 16

    struct hl_interface_param_s
    {
        /** */
        const char *device_desc;
        /** List of recognised VIDs */
        uint16_t vid[HLA_MAX_USB_IDS + 1];
        /** List of recognised PIDs */
        uint16_t pid[HLA_MAX_USB_IDS + 1];
        /** */
        enum hl_transports transport;
        /** */
        bool connect_under_reset;
        /** Initial interface clock clock speed */
        int initial_interface_speed;
        /** */
        bool use_stlink_tcp;
        /** */
        uint16_t stlink_tcp_port;
    };

    struct hl_interface_s
    {
        /** */
        struct hl_interface_param_s param;
        /** */
        const struct hl_layout *layout;
        /** */
        void *handle;
    };

    /** */
    struct hl_layout_api_s
    {
        /** */
        int (*open)(struct hl_interface_param_s *param, void **handle);
        /** */
        int (*close)(void *handle);
        /** */
        int (*reset)(void *handle);
        /** */
        int (*assert_srst)(void *handle, int srst);
        /** */
        int (*run)(void *handle);
        /** */
        int (*halt)(void *handle);
        /** */
        int (*step)(void *handle);
        /** */
        int (*read_regs)(void *handle);
        /**
	 * Read one register from the target
	 *
	 * @param handle A pointer to the device-specific handle
	 * @param regsel Register selection index compatible with all the
	 * values allowed by armv7m DCRSR.REGSEL
	 * @param val A pointer to retrieve the register value
	 * @returns ERROR_OK on success, or an error code on failure.
	 */
        int (*read_reg)(void *handle, unsigned int regsel, uint32_t *val);
        /**
	 * Write one register to the target
	 * @param handle A pointer to the device-specific handle
	 * @param regsel Register selection index compatible with all the
	 * values allowed by armv7m DCRSR.REGSEL
	 * @param val The value to be written in the register
	 * @returns ERROR_OK on success, or an error code on failure.
	 */
        int (*write_reg)(void *handle, unsigned int regsel, uint32_t val);
        /** */
        int (*read_mem)(void *handle, uint32_t addr, uint32_t size,
                        uint32_t count, uint8_t *buffer);
        /** */
        int (*write_mem)(void *handle, uint32_t addr, uint32_t size,
                         uint32_t count, const uint8_t *buffer);
        /** */
        int (*write_debug_reg)(void *handle, uint32_t addr, uint32_t val);
        /**
	 * Read the idcode of the target connected to the adapter
	 *
	 * If the adapter doesn't support idcode retrieval, this callback should
	 * store 0 to indicate a wildcard match.
	 *
	 * @param handle A pointer to the device-specific handle
	 * @param idcode Storage for the detected idcode
	 * @returns ERROR_OK on success, or an error code on failure.
	 */
        int (*idcode)(void *handle, uint32_t *idcode);
        /** */
        int (*override_target)(const char *targetname);
        /** */
        int (*custom_command)(void *handle, const char *command);
        /** */
        int (*speed)(void *handle, int khz, bool query);
        /**
	 * Configure trace parameters for the adapter
	 *
	 * @param handle A handle to adapter
	 * @param enabled Whether to enable trace
	 * @param pin_protocol Configured pin protocol
	 * @param port_size Trace port width for sync mode
	 * @param trace_freq A pointer to the configured trace
	 * frequency; if it points to 0, the adapter driver must write
	 * its maximum supported rate there
	 * @returns ERROR_OK on success, an error code on failure.
	 */
        int (*config_trace)(void *handle, bool enabled,
                            enum tpiu_pin_protocol pin_protocol, uint32_t port_size,
                            unsigned int *trace_freq, unsigned int traceclkin_freq,
                            uint16_t *prescaler);
        /**
	 * Poll for new trace data
	 *
	 * @param handle A handle to adapter
	 * @param buf A pointer to buffer to store received data
	 * @param size A pointer to buffer size; must be filled with
	 * the actual amount of bytes written
	 *
	 * @returns ERROR_OK on success, an error code on failure.
	 */
        int (*poll_trace)(void *handle, uint8_t *buf, size_t *size);
        /** */
        enum target_state (*state)(void *fd);
    };

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif /* PH_STLINK_H */
