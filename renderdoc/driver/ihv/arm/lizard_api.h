#ifndef LIB_LIZARD_API_H
#define LIB_LIZARD_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif    // End of __cplusplus

struct LizardApi;
typedef void *LizardInstance;
typedef uint32_t LizardCounterId;

typedef enum {
  LZD_OK = 0,
  LZD_FAILURE,
} LZD_RESULT;

/**
 * LizardInstance* LZD_Init(const char* host, int port);
 *
 * Initializes a Lizard Instance with the given host:port arguments.
 * The Lizard Instance must be destroyed with the LZD_Destroy method.
 *
 * :param host: IP address of the target gatord
 * :param port: Port number of the target gatord
 */
typedef LizardInstance *(*LZD_Init_PFN)(const char *host, int port);

/**
 * void LZD_Destroy(LizardInstance** ctx);
 *
 * Destroy the Lizard Instance and sets the `ctx` pointer's value to NULL.
 */
typedef void (*LZD_Destroy_PFN)(LizardInstance **ctx);

/**
 * Callback prototype to use for LZD_EnumerateCounters method.
 *
 * Parameters:
 *  LizardInstance* ctx: the LizardInstance on which the enumeration is invoked.
 *  LizardCounterId id: a numeric ID for the counter.
 *  const char* shortName: an internal short text identifier for the counter.
 *  void* userData: any user provided data (provided via the LZD_EnumerateCounters method).
 */
typedef void (*LZD_EnumerateCounters_Callback_PFN)(LizardInstance *ctx, LizardCounterId id,
                                                   const char *shortName, void *userData);

/**
 * Enumerate the available counters.
 *
 * If the `callback` parameter is `NULL` the method just returns the number of counters.
 *
 * :param ctx: Lizard Instance created via LZD_Init.
 * :param callback: Callback method invoked for each counter.
 * :param userData: User provided data which is passed to the callback.
 * :returns: Number of available counters.
 */
typedef uint32_t (*LZD_EnumerateCounters_PFN)(LizardInstance *ctx,
                                              LZD_EnumerateCounters_Callback_PFN callback,
                                              void *userData);

/**
 * Enable the counter for capture.
 *
 * :param ctx: Lizard Instance created via LZD_Init
 * :param id: Id of the counter to enable.
 */
typedef void (*LZD_EnableCounter_PFN)(LizardInstance *ctx, LizardCounterId id);

/**
 * Disable the counter for capture.
 *
 * :param ctx: Lizard Instance created via LZD_Init
 * :param id: Id of the counter to disable.
 */
typedef void (*LZD_DisableCounter_PFN)(LizardInstance *ctx, LizardCounterId id);

/**
 * Disable all counters for capture.
 *
 * By default all counters are disabled.
 *
 * :param ctx: Lizard Instance created via LZD_Init
 */
typedef void (*LZD_DisableAllCounters_PFN)(LizardInstance *ctx);

/**
 * Start capture.
 *
 * The actual capture is performed in a different thread.
 *
 * :param ctx: Lizard Instance created via LZD_Init
 */
typedef LZD_RESULT (*LZD_StartCapture_PFN)(LizardInstance *ctx);

/**
 * Stop capture.
 *
 * :param ctx: Lizard Instance created via LZD_Init
 */
typedef LZD_RESULT (*LZD_StopCapture_PFN)(LizardInstance *ctx);

typedef enum {
  LZD_ABSOLUTE = 1,
  LZD_DELTA = 2,
} LZD_CounterClassType;

struct LizardRawData
{
  int count;
  LZD_CounterClassType type;
  int64_t *values;
};

/**
 * Read back the given counter's raw data.
 *
 * :param ctx: Lizard Instance created via LZD_Init.
 * :param id: The id of the counter from which the data should be read.
 * :returns: A LizardData pointer which must be freed with LZD_FreeCounterData after it is not used.
 */
typedef LizardRawData *(*LZD_ReadRawCounter_PFN)(LizardInstance *ctx, LizardCounterId id);

typedef int64_t (*LZD_ReadCounter_PFN)(LizardInstance *ctx, LizardCounterId id);

typedef enum {
  /* String types */
  LZD_COUNTER_TITLE,
  LZD_COUNTER_NAME,
  LZD_COUNTER_CATEGORY,
  LZD_COUNTER_DESCRIPTION,
  LZD_COUNTER_KEY,

  /* Enum types (int) */
  LZD_COUNTER_CLASS_TYPE,
} LZD_CounterAttribute;

/**
 * Get the various Counter attribute values as a string.
 *
 * :param ctx: Lizard Instance created via LZD_INT
 * :param id: The id of the counter from which the data should be read.
 * :param attr: The attribute kind to retrive.
 * :returns: The value of the attribute or NULL.
 *           Note: the returned pointer is owned by the library not the caller, thus there is no
 * need to call free/delete on it.
 */
typedef const char *(*LZD_GetCounterStringAttribute_PFN)(LizardInstance *ctx, LizardCounterId id,
                                                         LZD_CounterAttribute attr);

typedef int32_t (*LZD_GetCounterIntAttribute_PFN)(LizardInstance *ctx, LizardCounterId id,
                                                  LZD_CounterAttribute attr);

/**
 * Releases a LizardData returned by the LZD_ReadRawCounter
 *
 * After this call the `data` will be set to `NULL`.
 *
 * :param data: LizardData to free.
 */
typedef void (*LZD_FreeCounterData_PFN)(LizardRawData **data);

struct LizardApi
{
  int struct_size;
  int version;
  LZD_Init_PFN Init;
  LZD_Destroy_PFN Destroy;
  LZD_EnumerateCounters_PFN EnumerateCounters;
  LZD_EnableCounter_PFN EnableCounter;
  LZD_DisableCounter_PFN DisableCounter;
  LZD_DisableAllCounters_PFN DisableAllCounters;

  LZD_StartCapture_PFN StartCapture;
  LZD_StopCapture_PFN StopCapture;

  LZD_ReadCounter_PFN ReadCounter;
  LZD_ReadRawCounter_PFN ReadRawCounter;
  LZD_GetCounterStringAttribute_PFN GetCounterStringAttribute;
  LZD_GetCounterIntAttribute_PFN GetCounterIntAttribute;
  LZD_FreeCounterData_PFN FreeCounterData;
};

/**
 * Entry point of the API.
 *
 * To load the api search for the "LoadApi" function symbol and
 * invoke the method with a `LizardApi` pointer to get access for all
 * API functions.
 *
 * Example usage:
 *
 *  void* lib = dlopen("liblizard.so", RTLD_LAZY);
 *  LZD_LoadApi_PFN loadApi = (LZD_LoadApi_PFN)dlsym(lib, "LoadApi");
 *
 *  struct LizardApi* api;
 *
 *  if (loadApi(&api) != LZD_OK) {
 *   // report failer and return
 *  }
 *
 *  LizardInstance* ctx = api->Init("127.0.0.1", 8080);
 *
 * :param api:
 * :returns: LZD_OK if the gator connection was ok.
 */
typedef LZD_RESULT (*LZD_LoadApi_PFN)(struct LizardApi **api);

#ifdef __cplusplus
}
#endif    // End of __cplusplus

#endif /* LIB_LIZARD_API_H */
