#

ifdef C_TAG
CFLAGS += -DTAG_FOR_C=$(C_TAG)
endif

ifdef T_TAG
CFLAGS += -DTAG_FOR_T=$(T_TAG)
endif

ifdef U_TAG
CFLAGS += -DTAG_FOR_U=$(U_TAG)
endif

ifdef C_CHUNK_SIZE
CFLAGS += -DCHUNK_FOR_C=$(C_CHUNK_SIZE)
endif

ifdef T_CHUNK_SIZE
CFLAGS += -DCHUNK_FOR_T=$(T_CHUNK_SIZE)
endif

ifdef U_CHUNK_SIZE
CFLAGS += -DCHUNK_FOR_U=$(U_CHUNK_SIZE)
endif

