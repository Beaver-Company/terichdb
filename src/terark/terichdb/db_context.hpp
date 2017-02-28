#ifndef __terichdb_db_context_hpp__
#define __terichdb_db_context_hpp__

#include "db_conf.hpp"

namespace terark {
	class BaseDFA;
}

namespace terark { namespace terichdb {

typedef boost::intrusive_ptr<class DbTable> DbTablePtr;
typedef boost::intrusive_ptr<class StoreIterator> StoreIteratorPtr;

template<class T>
struct DbContextObjCacheFreeExcessMem {
    static void invoke(T*) {}
};
template<>
struct DbContextObjCacheFreeExcessMem<valvec<byte>> {
    static void invoke(valvec<byte>* obj) {
        static size_t constexpr limit_size = 1024 * 1204;
        if (obj->size() > limit_size) {
            obj->clear();
        }
    }
};

template<class T>
class DbContextObjCache {
private:
    struct Wrapper {
        DbContextObjCache* owner;
        T x;
    };
    struct CacheItem {
        CacheItem(Wrapper *o) : obj(o) {}
        CacheItem(CacheItem const &) = delete;
        CacheItem(CacheItem &&o) : obj(o.obj) {
            o.obj = nullptr;
        }
        Wrapper* obj;

        T* operator->() const { return &obj->x; }
        T& operator*() const { return obj->x; }
        T* get() const { return &obj->x; }

        ~CacheItem() {
            if (obj) {
                DbContextObjCacheFreeExcessMem<T>::invoke(&obj->x);
                obj->owner->pool.emplace_back(obj);
            }
        }
    };
    valvec<Wrapper*> pool;
public:
    ~DbContextObjCache() {
        for (auto item : pool) {
            delete item;
        }
    }
    CacheItem get() {
        if (pool.empty()) {
            return CacheItem(new Wrapper{this, {}});
        }
        else {
            return CacheItem(pool.pop_val());
        }
    }
};

class TERICHDB_DLL DbContextLink : public RefCounter {
	friend class DbTable;
protected:
	DbContextLink();
	~DbContextLink();
//	DbContextLink *m_prev, *m_next;
};
class TERICHDB_DLL DbContext : public DbContextLink {
	friend class DbTable;
public:
	explicit DbContext(const DbTable* tab);
	~DbContext();

	void doSyncSegCtxNoLock(const DbTable* tab);
	void trySyncSegCtxNoLock(const DbTable* tab);
	void trySyncSegCtxSpeculativeLock(const DbTable* tab);
	class StoreIterator* getWrtStoreIterNoLock(size_t segIdx);
	class IndexIterator* getIndexIterNoLock(size_t segIdx, size_t indexId);

	void getWrSegWrtStoreData(const class ReadableSegment* seg, llong subId, valvec<byte>* buf);

	void debugCheckUnique(fstring row, size_t uniqIndexId);

/// @{ delegate methods
	StoreIteratorPtr createTableIterForward();
	StoreIteratorPtr createTableIterBackward();

	void getValueAppend(llong id, valvec<byte>* val);
	void getValue(llong id, valvec<byte>* val);

	llong insertRow(fstring row);
	llong upsertRow(fstring row);
	llong updateRow(llong id, fstring row);
	void  removeRow(llong id);

	void indexInsert(size_t indexId, fstring indexKey, llong id);
	void indexRemove(size_t indexId, fstring indexKey, llong id);
	void indexUpdate(size_t indexId, fstring indexKey, llong oldId, llong newId);

	void indexSearchExact(size_t indexId, fstring key, valvec<llong>* recIdvec);
	bool indexKeyExists(size_t indexId, fstring key);

	void indexSearchExactNoLock(size_t indexId, fstring key, valvec<llong>* recIdvec);
	bool indexKeyExistsNoLock(size_t indexId, fstring key);

	bool indexMatchRegex(size_t indexId, class RegexForIndex*, valvec<llong>* recIdvec);

	void selectColumns(llong id, const valvec<size_t>& cols, valvec<byte>* colsData);
	void selectColumns(llong id, const size_t* colsId, size_t colsNum, valvec<byte>* colsData);
	void selectOneColumn(llong id, size_t columnId, valvec<byte>* colsData);

	void selectColgroups(llong id, const valvec<size_t>& cgIdvec, valvec<valvec<byte> >* cgDataVec);
	void selectColgroups(llong id, const size_t* cgIdvec, size_t cgIdvecSize, valvec<byte>* cgDataVec);

	void selectOneColgroup(llong id, size_t cgId, valvec<byte>* cgData);

	void selectColumnsNoLock(llong id, const valvec<size_t>& cols, valvec<byte>* colsData);
	void selectColumnsNoLock(llong id, const size_t* colsId, size_t colsNum, valvec<byte>* colsData);
	void selectOneColumnNoLock(llong id, size_t columnId, valvec<byte>* colsData);

	void selectColgroupsNoLock(llong id, const valvec<size_t>& cgIdvec, valvec<valvec<byte> >* cgDataVec);
	void selectColgroupsNoLock(llong id, const size_t* cgIdvec, size_t cgIdvecSize, valvec<byte>* cgDataVec);

	void selectOneColgroupNoLock(llong id, size_t cgId, valvec<byte>* cgData);
/// @}

	class ReadableSegment* getSegmentPtr(size_t segIdx) const;

	void ensureTransactionNoLock();
	void freeWritableSegmentResources();

public:
	struct SegCtx {
		class ReadableSegment* seg;
		class StoreIterator* wrtStoreIter;
		class IndexIterator* indexIter[1];
	private:
		friend class DbContext;
		~SegCtx() = delete;
		SegCtx() = delete;
		SegCtx(const SegCtx&) = delete;
		SegCtx& operator=(const SegCtx&) = delete;
		static SegCtx* create(ReadableSegment* seg, size_t indexNum);
		static void destory(SegCtx*& p, size_t indexNum);
		static void reset(SegCtx* p, size_t indexNum, ReadableSegment* seg);
	};
	DbTable* m_tab;
	class WritableSegment* m_wrSegPtr;
	std::unique_ptr<class DbTransaction> m_transaction;
	valvec<SegCtx*> m_segCtx;
	valvec<llong>   m_rowNumVec; // copy of DbTable::m_rowNumVec
	llong           m_mySnapshotVersion;
	std::string  errMsg;
    DbContextObjCache<valvec<byte>> bufs;
    DbContextObjCache<ColumnVec> cols;
	valvec<uint32_t> offsets;
	valvec<llong> exactMatchRecIdvec;
    boost::intrusive_ptr<RefCounter> trbLog;
	size_t regexMatchMemLimit;
	size_t segArrayUpdateSeq;
	int  upsertMaxRetry;
	bool syncIndex;
	bool m_isUserDefineSnapshot;
    bool syncOnCommit;
	byte isUpsertOverwritten;
};
typedef boost::intrusive_ptr<DbContext> DbContextPtr;

} } // namespace terark::terichdb

#endif // __terichdb_db_context_hpp__
