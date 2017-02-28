/**
 *    Copyright (C) 2016 Terark Inc.
 *    This file is heavily modified based on MongoDB WiredTiger StorageEngine
 *    Created on: 2015-12-01
 *    Author    : leipeng, rockeet@gmail.com
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include <set>
#include <string>

#include "mongo_terichdb_common.hpp"

#include "mongo/bson/ordering.h"
#include "mongo/db/storage/kv/kv_engine.h"
//#include "terichdb_session_cache.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/elapsed_tracker.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_kv_engine.h"

#include <terark/hash_strmap.hpp>
#include "terichdb_size_storer.h"

namespace mongo {
	class KVCatalog; // fuckMongoKVCatalog
}

namespace mongo { namespace db {

class TerichDbKVEngine final : public KVEngine {
public:
    TerichDbKVEngine(const std::string& path,
				   const std::string& extraOpenOptions,
				   size_t cacheSizeGB,
				   bool durable,
				   bool repair);
    virtual ~TerichDbKVEngine();

    void setRecordStoreExtraOptions(const std::string& options);
    void setSortedDataInterfaceExtraOptions(const std::string& options);

    virtual bool supportsDocLocking() const override;

    virtual bool supportsDirectoryPerDB() const override;

    virtual bool isDurable() const override {
        return _durable;
    }

    virtual bool isEphemeral() const override { return false; }

    virtual RecoveryUnit* newRecoveryUnit() override;

    virtual Status createRecordStore(OperationContext* opCtx,
                                     StringData ns,
                                     StringData ident,
                                     const CollectionOptions& options) override;

    virtual RecordStore* getRecordStore(OperationContext* opCtx,
                                        StringData ns,
                                        StringData ident,
                                        const CollectionOptions& options) override;

    virtual Status createSortedDataInterface(OperationContext* opCtx,
                                             StringData ident,
                                             const IndexDescriptor* desc) override;

    virtual SortedDataInterface* getSortedDataInterface(OperationContext* opCtx,
                                                        StringData ident,
                                                        const IndexDescriptor* desc) override;

    virtual Status dropIdent(OperationContext* opCtx, StringData ident) override;

    virtual Status okToRename(OperationContext* opCtx,
                              StringData fromNS,
                              StringData toNS,
                              StringData ident,
                              const RecordStore* originalRecordStore) const override;

    virtual int flushAllFiles(bool sync) override;

    virtual Status beginBackup(OperationContext* txn) override;

    virtual void endBackup(OperationContext* txn) override;

    virtual int64_t getIdentSize(OperationContext* opCtx, StringData ident) override;

    virtual Status repairIdent(OperationContext* opCtx, StringData ident) override;

    virtual bool hasIdent(OperationContext* opCtx, StringData ident) const override;

    std::vector<std::string> getAllIdents(OperationContext* opCtx) const override;

    virtual void cleanShutdown() override;

    virtual void setJournalListener(JournalListener* jl) override;

    // terichdb specific
    // Calls TerichDb_CONNECTION::reconfigure on the underlying TerichDb_CONNECTION
    // held by this class
    int reconfigure(const char* str);

	const KVCatalog* m_fuckKVCatalog;

private:
//  std::unique_ptr<WiredTigerSessionCache> _sessionCache;
    std::string _path;
    fs::path m_pathTerark;
    fs::path m_pathTerarkTables;

    // for: 1. capped collection
    //      2. metadata(use wiredtiger)
    //      3. ephemeral table/index, ephemeral will use WiredTigerKVEngine
    fs::path m_pathWt;
    std::unique_ptr<WiredTigerKVEngine> m_wtEngine;

    typedef terark::hash_strmap<ThreadSafeTablePtr> TableMap;

    mutable TableMap m_tables;
    struct TableIndex {
		~TableIndex() {}
    	size_t indexId;
    	ThreadSafeTablePtr m_table;
    	SortedDataInterface* m_index = nullptr;
    };

    typedef terark::hash_strmap<TableIndex> IndexMap;
    IndexMap m_indices;
	mutable std::mutex m_mutex;

	bool m_identAsDir;
    bool _durable;

    std::string _rsOptions;
    std::string _indexOptions;

    std::set<std::string> _identToDrop;
    mutable stdx::mutex _identToDropMutex;

    TerichDbSizeStorer _sizeStorer;
    mutable ElapsedTracker _sizeStorerSyncTracker;

    mutable Date_t _previousCheckedDropsQueued;

	std::unique_ptr<TableMap> _backupSession;

	ThreadSafeTable* openTable(StringData ns, StringData ident);
	boost::filesystem::path getTableDir(StringData ns, StringData ident) const;
};
} }  // namespace mongo::terark

