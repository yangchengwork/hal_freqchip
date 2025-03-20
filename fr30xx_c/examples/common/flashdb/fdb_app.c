#include <stdio.h>

#include "fr30xx.h"
#include "flashdb.h"

/* KVDB object */
static struct fdb_kvdb kvdb={0};

/* critical zone protection */
static CPU_SR cpu_sr = 0;

static void lock(fdb_db_t db)
{
    /* used to avoid reentry */
    if (cpu_sr & 0xff00) {
        while(1);
    }

    cpu_sr = CPU_SR_Save(0x20);
    cpu_sr |= 0xff00;
}

static void unlock(fdb_db_t db)
{
    cpu_sr &= 0xff;
    CPU_SR_Restore(cpu_sr);
}

int flashdb_init(void)
{
    fdb_err_t result;

    /* set the lock and unlock function if you want */
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);

    /* Key-Value database initialization
     *
     *       &kvdb: database object
     *       "env": database name
     * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
     *              Please change to YOUR partition name.
     * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
     *        NULL: The user data if you need, now is empty.
     */
    
    result = fdb_kvdb_init(&kvdb, "env", "FlashEnv", NULL, NULL);

    if (result != FDB_NO_ERR) {
        return -1;
    }

    return 0;
}

fdb_err_t flashdb_set(uint32_t key, uint8_t *value, uint32_t length)
{
    struct fdb_blob blob;

    return fdb_kv_set_blob(&kvdb, key, fdb_blob_make(&blob, value, length));
}

size_t flashdb_get(uint32_t key, uint8_t *value, uint32_t length)
{
    struct fdb_blob blob;

    return fdb_kv_get_blob(&kvdb, key, fdb_blob_make(&blob, value, length));
}

size_t flashdb_get_length(uint32_t key)
{
    struct fdb_kv kv;

    if (fdb_kv_get_obj(&kvdb, key, &kv) == NULL) {
        return 0;
    }
    else {
        return kv.value_len;
    }
}

fdb_err_t flashdb_del(uint32_t key)
{
    return fdb_kv_del(&kvdb, key);
}

