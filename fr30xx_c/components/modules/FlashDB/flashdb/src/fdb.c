/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Initialize interface.
 *
 * Some initialize interface for this library.
 */

#include <flashdb.h>
#include <fdb_low_lvl.h>
#include <string.h>

#define FDB_LOG_TAG ""

#if !defined(FDB_USING_FAL_MODE) && !defined(FDB_USING_FILE_MODE)
#error "Please defined the FDB_USING_FAL_MODE or FDB_USING_FILE_MODE macro"
#endif

fdb_err_t _fdb_init_ex(fdb_db_t db, const char *name, const char *path, fdb_db_type type, void *user_data)
{
    FDB_ASSERT(db);
    FDB_ASSERT(name);
    FDB_ASSERT(path);

    if (db->init_ok) {
        return FDB_NO_ERR;
    }

    db->name = name;
    db->type = type;
    db->user_data = user_data;

    if (db->file_mode) {
#ifdef FDB_USING_FILE_MODE
        /* must set when using file mode */
        FDB_ASSERT(db->sec_size != 0);
        FDB_ASSERT(db->max_size != 0);
#ifdef FDB_USING_FILE_POSIX_MODE
        db->cur_file = -1;
#else
        db->cur_file = 0;
#endif
        db->storage.dir = path;
        FDB_ASSERT(strlen(path) != 0)
#endif
    } else {
#ifdef FDB_USING_FAL_MODE
        size_t block_size;

        /* FAL (Flash Abstraction Layer) initialization */
        fal_init();

        /* check the flash partition */
        if ((db->storage.part = fal_partition_find(path)) == NULL) {
            FDB_INFO("Error: Partition (%s) not found.\n", path);
            return FDB_PART_NOT_FOUND;
        }

        block_size = fal_flash_device_find(db->storage.part->flash_name)->blk_size;
        if (db->sec_size == 0) {
            db->sec_size = block_size;
        } else {
            /* must be aligned with block size */
            FDB_ASSERT(db->sec_size % block_size == 0);
        }

        db->max_size = db->storage.part->len;
#endif /* FDB_USING_FAL_MODE */
    }

    /* must align with sector size */
    FDB_ASSERT(db->max_size % db->sec_size == 0);
    /* must have more than or equal 2 sector */
    FDB_ASSERT(db->max_size / db->sec_size >= 2);

    return FDB_NO_ERR;
}

void _fdb_init_finish(fdb_db_t db, fdb_err_t result)
{
    static bool log_is_show = false;
    if (result == FDB_NO_ERR) {
        db->init_ok = true;
        if (!log_is_show) {
            FDB_INFO("FlashDB V%s is initialize success.\n", FDB_SW_VERSION);
            FDB_INFO("You can get the latest version on https://github.com/armink/FlashDB .\n");
            log_is_show = true;
        }
    } else if (!db->not_formatable) {
        FDB_INFO("Error: %s (%s) is initialize fail (%d).\n", db->type == FDB_DB_TYPE_KV ? "KVDB" : "TSDB",
                db->name, (int)result);
    }
}

void _fdb_deinit(fdb_db_t db)
{
    FDB_ASSERT(db);

    if (db->init_ok) {
#ifdef FDB_USING_FILE_MODE
#ifdef FDB_USING_FILE_POSIX_MODE
        if (db->cur_file > 0) {
#if !defined(_MSC_VER)
#include <unistd.h>
#endif
            close(db->cur_file);
        }
#else
        if (db->cur_file != 0) {
            fclose(db->cur_file);
        }
#endif /* FDB_USING_FILE_POSIX_MODE */
#endif /* FDB_USING_FILE_MODE */
    }

    db->init_ok = false;
}
