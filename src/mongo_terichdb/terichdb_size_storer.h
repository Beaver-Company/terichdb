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

#ifdef _MSC_VER
#pragma warning(disable: 4800) // bool conversion
#pragma warning(disable: 4244) // 'return': conversion from '__int64' to 'double', possible loss of data
#pragma warning(disable: 4267) // '=': conversion from 'size_t' to 'int', possible loss of data
#endif

#include "mongo_terichdb_common.hpp"
#include "mongo/base/string_data.h"
#include <map>
#include <string>
#include <mutex>
#include <terark/hash_strmap.hpp>
#include <terark/terichdb/db_table.hpp>

namespace mongo {
	class RecordStore;
} // namespace mongo

namespace mongo { namespace db {

class TerichDbSizeStorer {
public:
    TerichDbSizeStorer();
    ~TerichDbSizeStorer();

    void setFilePath(const std::string& fpath) { m_filepath = fpath; }
    const std::string& getFilePath() { return m_filepath; }
    void onCreate(RecordStore* rs, llong nr, llong ds);
    void onDestroy(RecordStore* rs);

    void storeToCache(fstring ns, llong numRecords, llong dataSize);
    void loadFromCache(fstring ns, llong* numRecords, llong* dataSize) const;

    void fillCache();
    void syncCache(bool syncToDisk);

    struct Entry {
        Entry() : numRecords(0), dataSize(0), rs(NULL), dirty(false) {}
        llong numRecords;
        llong dataSize;
        RecordStore* rs;  // not owned
        bool dirty;
        static const size_t MySize = 2 * sizeof(llong);
    };
private:
    std::string m_filepath;
    terark::hash_strmap<Entry> m_entries;
    mutable stdx::mutex m_mutex;
};
} }  // namespace mongo::terark

