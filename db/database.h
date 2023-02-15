#include <memory>
#include <lmdb.h>
#include <iostream>

namespace lmdb {
    //! Closes LMDB environment handle.
    struct close_env
    {
        void operator()(MDB_env* ptr) const noexcept
        {
            if (ptr)
                mdb_env_close(ptr);
        }
    };

    using environment = std::unique_ptr<MDB_env, close_env>;

    //! \return LMDB environment at `path` with a max of `max_dbs` tables.
    environment open_environment(const char* path, MDB_dbi max_dbs) noexcept;

    class database
    {

    };
}