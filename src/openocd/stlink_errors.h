#ifndef PH_STLINK_ERROR_H
#define PH_STLINK_ERROR_H

/* general failures
 * error codes < 100
 */
#define ERROR_OK (0)
#define ERROR_NO_CONFIG_FILE (-2)
#define ERROR_BUF_TOO_SMALL (-3)

/* see "Error:" log entry for meaningful message to the user. The caller should
 * make no assumptions about what went wrong and try to handle the problem.
 */
#define ERROR_FAIL (-4)
#define ERROR_WAIT (-5)

/* ERROR_TIMEOUT is already taken by winerror.h. */
#define ERROR_TIMEOUT_REACHED (-6)
#define ERROR_NOT_IMPLEMENTED (-7)

/* PH taken from command.h */
#define ERROR_COMMAND_CLOSE_CONNECTION (-600)
#define ERROR_COMMAND_SYNTAX_ERROR (-601)
#define ERROR_COMMAND_NOTFOUND (-602)
#define ERROR_COMMAND_ARGUMENT_INVALID (-603)
#define ERROR_COMMAND_ARGUMENT_OVERFLOW (-604)
#define ERROR_COMMAND_ARGUMENT_UNDERFLOW (-605)

/* PH taken from target.h */
#define ERROR_TARGET_INVALID (-300)
#define ERROR_TARGET_INIT_FAILED (-301)
#define ERROR_TARGET_TIMEOUT (-302)
#define ERROR_TARGET_NOT_HALTED (-304)
#define ERROR_TARGET_FAILURE (-305)
#define ERROR_TARGET_UNALIGNED_ACCESS (-306)
#define ERROR_TARGET_DATA_ABORT (-307)
#define ERROR_TARGET_RESOURCE_NOT_AVAILABLE (-308)
#define ERROR_TARGET_TRANSLATION_FAULT (-309)
#define ERROR_TARGET_NOT_RUNNING (-310)
#define ERROR_TARGET_NOT_EXAMINED (-311)
#define ERROR_TARGET_DUPLICATE_BREAKPOINT (-312)
#define ERROR_TARGET_ALGO_EXIT (-313)

#endif /* PH_STLINK_ERROR_H */
