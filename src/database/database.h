#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "connection_pool.h"
#include "db_connection_settings.h"
#include "player_record.h"

namespace postgres {

class PlayerRecordRepositoryImpl : public PlayerRecordRepository {
   public:
    explicit PlayerRecordRepositoryImpl(db::ConnectionPool& connection_pool)
        : connection_pool_(connection_pool){};

    void SaveRecordsTable(const std::vector<PlayerRecord>& player_records) override;
    std::vector<PlayerRecord> GetRecordsTable(size_t offset, size_t limit) override;

   private:
    db::ConnectionPool& connection_pool_;
};

class Database {
   public:
    explicit Database(const DbConnectrioSettings& db_settings);

    PlayerRecordRepositoryImpl& GetPlayerRecordRepository() & {
        return player_records_;
    }

   private:
    db::ConnectionPool connection_pool_;
    PlayerRecordRepositoryImpl player_records_{connection_pool_};
};

}  // namespace postgres