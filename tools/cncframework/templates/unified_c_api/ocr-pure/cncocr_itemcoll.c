{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

#include "cncocr_internal.h"

#define CNC_ITEMS_PER_BLOCK 64

#define CNC_ITEM_BLOCK_FULL(block) ((block)->count == CNC_ITEMS_PER_BLOCK)
#define CNC_GETTER_GUID ((ocrGuid_t)-1)

#define SIMPLE_DBCREATE(guid, ptr, sz) ocrDbCreate(guid, ptr, sz, DB_PROP_NONE, NULL_GUID, NO_ALLOC)

typedef struct {
    ocrGuid_t entry;
    ocrGuid_t bucketHead;
    ocrGuid_t firstBlock;
    ocrGuid_t oldFirstBlock;
    ocrGuid_t affinity;
    u32 firstBlockCount;
    u32 tagLength;
    u32 slot;
    ocrDbAccessMode_t mode;
    u8 checkedFirst;
    u64 role; // forcing tag[] to be 8-byte aligned (for FSim)
    cncTag_t tag[];
} ItemCollOpParams;

typedef struct {
    bool isEvent;
    ocrGuid_t guid;
} ItemBlockEntry;

typedef struct {
    u32 count;
    ocrGuid_t next;
    ItemBlockEntry entries[CNC_ITEMS_PER_BLOCK];
    cncTag_t tags[];
} ItemBlock;

static ocrGuid_t _itemBlockCreate(u32 tagLength, ocrGuid_t next, ItemBlock **out) {
    ocrGuid_t blockGuid;
    ItemBlock *block;
    u64 size = sizeof(ItemBlock) + (tagLength * sizeof(*block->tags) * CNC_ITEMS_PER_BLOCK);
    SIMPLE_DBCREATE(&blockGuid, (void**)&block, size);
    // XXX - should we start from the back?
    block->count = 0;
    block->next = next;
    *out = block;
    return blockGuid;
}

static ocrGuid_t _itemBlockInsert(ItemBlock *block, cncTag_t *tag, ocrGuid_t entry, u32 tagLength) {
    ASSERT(!CNC_ITEM_BLOCK_FULL(block));
    u32 i = block->count;
    if (entry == CNC_GETTER_GUID) {
        block->entries[i].isEvent = true;
        ocrEventCreate(&block->entries[i].guid, OCR_EVENT_IDEM_T, true);
    }
    else {
        block->entries[i].isEvent = false;
        block->entries[i].guid = entry;
    }
    hal_memCopy(&block->tags[i*tagLength], tag, tagLength*sizeof(*tag), 0);
    block->count += 1;
    return block->entries[i].guid;
}

static u32 _itemBlockFind(ItemBlock *block, cncTag_t *tag, u32 tagLength, u32 startAt) {
    u32 i = startAt;
    for (; i<block->count; i++) {
        if (_cncTagEquals(&block->tags[i*tagLength], tag, tagLength)) {
            return i;
        }
    }
    return CNC_ITEMS_PER_BLOCK; // not found
}

static ocrGuid_t _searchBucketEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]);

static ocrGuid_t _addToBucketEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // unpack
    ocrGuid_t *blockArray = depv[0].ptr;
    ItemCollOpParams *params = depv[1].ptr;
    ocrGuid_t paramsGuid = depv[1].guid;
    const u32 index = 0;
    // look up the first block the bucket
    ocrGuid_t firstBlock = blockArray[index];
    // is our first block still first?
    if (firstBlock == params->firstBlock) {
        ItemBlock *newFirst;
        ocrGuid_t blockGuid = _itemBlockCreate(params->tagLength, firstBlock, &newFirst);
        blockArray[index] = blockGuid;
        // XXX - repeated code, also in addToBlock
        bool isGetter = (params->role == _CNC_GETTER_ROLE);
        ocrGuid_t src = isGetter ? CNC_GETTER_GUID : params->entry;
        ocrGuid_t res = _itemBlockInsert(newFirst, params->tag, src, params->tagLength);
        ocrDbRelease(blockGuid);
        if (isGetter) {
            ocrAddDependence(res, params->entry, params->slot, params->mode);
        }
        // DONE! clean up.
        ocrDbDestroy(paramsGuid);
    }
    else { // someone added a new block...
        // try searching again
        params->oldFirstBlock = params->firstBlock;
        params->firstBlock = firstBlock;
        params->checkedFirst = 0;
        ocrGuid_t deps[] = { firstBlock, paramsGuid };
        ocrGuid_t searchEdtGuid, templGuid;
        ocrEdtTemplateCreate(&templGuid, _searchBucketEdt, 0, 2);
        ocrEdtCreate(&searchEdtGuid, templGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/deps,
            /*properties=*/EDT_PROP_NONE,
            /*affinity=*/params->affinity, /*outEvent=*/NULL);
        ocrEdtTemplateDestroy(templGuid);
    }
    return NULL_GUID;
}

static ocrGuid_t _addToBlockEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // unpack
    ItemBlock *block = depv[0].ptr;
    ItemCollOpParams *params = depv[1].ptr;
    ocrGuid_t paramsGuid = depv[1].guid;
    // is it in this block?
    // XXX - repeated code (also in the searchEdt)
    u32 i = _itemBlockFind(block, params->tag, params->tagLength, 0);
    if (i < CNC_ITEMS_PER_BLOCK) { // found!
        ocrGuid_t foundEntry = block->entries[i].guid;
        if (params->role == _CNC_GETTER_ROLE) { // Get
            ocrAddDependence(foundEntry, params->entry, params->slot, params->mode);
        }
        else if (block->entries[i].isEvent) { // Put
            ocrAddDependence(params->entry, foundEntry, 0, DB_DEFAULT_MODE);
        }
        // XXX - currently ignoring single assignment checks
        // DONE! clean up.
        ocrDbDestroy(paramsGuid);
    }
    // add the entry if there's still room
    else if (!CNC_ITEM_BLOCK_FULL(block)) {
        bool isGetter = (params->role == _CNC_GETTER_ROLE);
        ocrGuid_t src = isGetter ? CNC_GETTER_GUID : params->entry;
        ocrGuid_t res = _itemBlockInsert(block, params->tag, src, params->tagLength);
        if (isGetter) {
            ocrAddDependence(res, params->entry, params->slot, params->mode);
        }
        // DONE! clean up.
        ocrDbDestroy(paramsGuid);
    }
    else { // the block filled up while we were searching
        // might need to add a new block to the bucket
        ocrGuid_t addEdtGuid, templGuid;
        ocrEdtTemplateCreate(&templGuid, _addToBucketEdt, 0, 2);
        ocrEdtCreate(&addEdtGuid, templGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/NULL,
            /*properties=*/EDT_PROP_NONE,
            /*affinity=*/params->affinity, /*outEvent=*/NULL);
        ocrAddDependence(params->bucketHead, addEdtGuid, 0, DB_MODE_EW);
        ocrAddDependence(paramsGuid, addEdtGuid, 1, DB_DEFAULT_MODE);
        ocrEdtTemplateDestroy(templGuid);
    }
    return NULL_GUID;
}

static ocrGuid_t _searchBucketEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // unpack
    ItemBlock *block = depv[0].ptr;
    ocrGuid_t blockGuid = depv[0].guid;
    ItemCollOpParams *params = depv[1].ptr;
    ocrGuid_t paramsGuid = depv[1].guid;
    // record first block size (if applicable)
    if (!params->checkedFirst) {
        params->checkedFirst = 1;
        params->firstBlockCount = block->count;
    }
    // is it in this block?
    u32 i = _itemBlockFind(block, params->tag, params->tagLength, 0);
    if (i < CNC_ITEMS_PER_BLOCK) { // found!
        ocrGuid_t foundEntry = block->entries[i].guid;
        if (params->role == _CNC_GETTER_ROLE) { // Get
            ocrAddDependence(foundEntry, params->entry, params->slot, params->mode);
        }
        else if (block->entries[i].isEvent) { // Put
            ocrAddDependence(params->entry, foundEntry, 0, DB_DEFAULT_MODE);
        }
        // XXX - currently ignoring single assignment checks
        // DONE! clean up.
        ocrDbDestroy(paramsGuid);
    }
    // did we reach the end of the search?
    else if (block->next == NULL_GUID || blockGuid == params->oldFirstBlock) {
        // try to go back and add it to the first block
        // XXX - should check if it was full
        ocrGuid_t addEdtGuid, templGuid;
        ocrEdtTemplateCreate(&templGuid, _addToBlockEdt, 0, 2);
        ocrEdtCreate(&addEdtGuid, templGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/NULL,
            /*properties=*/EDT_PROP_NONE,
            /*affinity=*/params->affinity, /*outEvent=*/NULL);
        ocrAddDependence(params->firstBlock, addEdtGuid, 0, DB_MODE_EW);
        ocrAddDependence(paramsGuid, addEdtGuid, 1, DB_DEFAULT_MODE);
        ocrEdtTemplateDestroy(templGuid);
    }
    else { // keep looking...
        // search next
        ocrGuid_t deps[] = { block->next, paramsGuid };
        ocrGuid_t searchEdtGuid, templGuid;
        ocrEdtTemplateCreate(&templGuid, _searchBucketEdt, 0, 2);
        ocrEdtCreate(&searchEdtGuid, templGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/deps,
            /*properties=*/EDT_PROP_NONE,
            /*affinity=*/params->affinity, /*outEvent=*/NULL);
        ocrEdtTemplateDestroy(templGuid);
    }
    return NULL_GUID;
}

static ocrGuid_t _bucketHeadEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // unpack
    ocrGuid_t *blockArray = depv[0].ptr;
    ItemCollOpParams *params = depv[1].ptr;
    ocrGuid_t paramsGuid = depv[1].guid;
    // save bucket info for first block
    params->firstBlock = blockArray[0];
    if (params->firstBlock == NULL_GUID) { // empty bucket
        // might need to add a new block to the bucket
        ocrGuid_t addEdtGuid, templGuid;
        ocrEdtTemplateCreate(&templGuid, _addToBucketEdt, 0, 2);
        ocrEdtCreate(&addEdtGuid, templGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/NULL,
            /*properties=*/EDT_PROP_NONE,
            /*affinity=*/params->affinity, /*outEvent=*/NULL);
        ocrAddDependence(params->bucketHead, addEdtGuid, 0, DB_MODE_EW);
        ocrAddDependence(paramsGuid, addEdtGuid, 1, DB_DEFAULT_MODE);
        ocrEdtTemplateDestroy(templGuid);
    }
    else { // search the bucket
        ocrGuid_t deps[] = { params->firstBlock, paramsGuid };
        ocrGuid_t searchEdtGuid, templGuid;
        ocrEdtTemplateCreate(&templGuid, _searchBucketEdt, 0, 2);
        ocrEdtCreate(&searchEdtGuid, templGuid,
            /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
            /*depc=*/EDT_PARAM_DEF, /*depv=*/deps,
            /*properties=*/EDT_PROP_NONE,
            /*affinity=*/params->affinity, /*outEvent=*/NULL);
        ocrEdtTemplateDestroy(templGuid);
    }
    return NULL_GUID;
}

static ocrGuid_t _doHashEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // unpack
    ocrGuid_t *blockArray = depv[0].ptr;
    ItemCollOpParams *params = depv[1].ptr;
    ocrGuid_t paramsGuid = depv[1].guid;
    // find the the bucket index
    u64 hash = _cncTagHash(params->tag, params->tagLength);
    u32 index = hash % CNC_TABLE_SIZE;
    // save bucket info
    params->bucketHead = blockArray[index];
    params->oldFirstBlock = NULL_GUID;
    params->checkedFirst = 0;
    // go into bucket
    ocrGuid_t deps[] = { params->bucketHead, paramsGuid };
    ocrGuid_t bucketEdtGuid, templGuid;
    ocrEdtTemplateCreate(&templGuid, _bucketHeadEdt, 0, 2);
    ocrEdtCreate(&bucketEdtGuid, templGuid,
        /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
        /*depc=*/EDT_PARAM_DEF, /*depv=*/deps,
        /*properties=*/EDT_PROP_NONE,
        /*affinity=*/params->affinity, /*outEvent=*/NULL);
    ocrEdtTemplateDestroy(templGuid);
    return NULL_GUID;
}

void _cncItemCollUpdate(cncItemCollHandle_t handle, cncTag_t *tag, u32 tagLength, u8 role,
        ocrGuid_t input, u32 slot, ocrDbAccessMode_t mode) {
    // Build datablock to hold search parameters
    ocrGuid_t paramsGuid;
    ItemCollOpParams *params;
    const size_t tagBytes = tagLength*sizeof(*tag);
    SIMPLE_DBCREATE(&paramsGuid, (void**)&params, sizeof(ItemCollOpParams) + tagBytes);
    params->entry = input;
    params->tagLength = tagLength;
    params->role = role;
    params->slot = slot;
    params->mode = mode;
    hal_memCopy(params->tag, tag, tagBytes, 0);
    // affinity
    // TODO - affinitizing each bucket with node, and running all the
    // lookup stuff on that node would probably be cheaper.
    #ifdef CNC_AFFINITIES
    ocrAffinityGetCurrent(&params->affinity);
    #else
    params->affinity = NULL_GUID;
    #endif /* CNC_AFFINITIES */
    ocrDbRelease(paramsGuid);
    // edt
    ocrGuid_t hashEdtGuid, templGuid;
    ocrEdtTemplateCreate(&templGuid, _doHashEdt, 0, 2);
    ocrEdtCreate(&hashEdtGuid, templGuid,
        /*paramc=*/EDT_PARAM_DEF, /*paramv=*/NULL,
        /*depc=*/EDT_PARAM_DEF, /*depv=*/NULL,
        /*properties=*/EDT_PROP_NONE,
        /*affinity=*/params->affinity, /*outEvent=*/NULL);
    ocrAddDependence(handle.coll, hashEdtGuid, 0, DB_MODE_RO);
    ocrAddDependence(paramsGuid, hashEdtGuid, 1, DB_DEFAULT_MODE);
    ocrEdtTemplateDestroy(templGuid);
}

cncItemCollection_t _cncItemCollectionCreate(void) {
    int i;
    ocrGuid_t collGuid, *itemTable;
    SIMPLE_DBCREATE(&collGuid, (void**)&itemTable, sizeof(ocrGuid_t)*CNC_TABLE_SIZE);
    for (i=0; i<CNC_TABLE_SIZE; i++) {
        ocrGuid_t *_ptr;
        // Add one level of indirection to help with contention
        SIMPLE_DBCREATE(&itemTable[i], (void**)&_ptr, sizeof(ocrGuid_t));
        *_ptr = NULL_GUID;
        ocrDbRelease(itemTable[i]);
    }
    ocrDbRelease(collGuid);
    return collGuid;
}

void _cncItemCollectionDestroy(cncItemCollection_t coll) {
    // FIXME - need to do a deep traversal to really destroy the collection
    ocrDbDestroy(coll);
}

cncItemCollection_t _cncItemCollectionSingletonCreate(void) {
    ocrGuid_t coll;
    // FIXME - 1 below should be a property name...
    ocrEventCreate(&coll, OCR_EVENT_IDEM_T, 1);
    return coll;
}

void _cncItemCollectionSingletonDestroy(cncItemCollection_t coll) {
    // FIXME - need to do a deep traversal to really destroy the collection
    ocrEventDestroy(coll);
}

