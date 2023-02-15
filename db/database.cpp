#include "database.h"

#include <sys/stat.h>

namespace
{
    constexpr const mdb_mode_t open_flags = (S_IRUSR | S_IWUSR);
}

namespace lmdb{
    environment open_environment(const char* path, MDB_dbi max_dbs) noexcept
    {

        MDB_env* obj = nullptr;
        mdb_env_create(std::addressof(obj));
        environment out{obj};

        mdb_env_set_maxdbs(out.get(), max_dbs);

        if(mdb_env_open(out.get(), path, 0, open_flags))
        {
            std::cout <<"db created : " << path <<"\n";
        }

        return {std::move(out)};
    }
}