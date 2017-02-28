#ifndef __terichdb_segment_hpp__
#define __terichdb_segment_hpp__

#include "db_index.hpp"
#include "db_store.hpp"
#include <terark/bitmap.hpp>
#include <terark/rank_select.hpp>
#include <tbb/spin_rw_mutex.h>
#include <tbb/tbb_thread.h>

namespace terark {
	class SortableStrVec;
}

namespace terark { namespace terichdb {

typedef tbb::spin_rw_mutex        SpinRwMutex;
typedef SpinRwMutex::scoped_lock  SpinRwLock;

// This ReadableStore is used for return full-row
// A full-row is of one table, the table has multiple indices
class TERICHDB_DLL ReadableSegment : public ReadableStore {
public:
	struct TERICHDB_DLL RegisterSegmentFactory {
		typedef std::function<ReadableSegment*()> SegmentCreator;
		RegisterSegmentFactory(std::initializer_list<fstring> names, const SegmentCreator&);
	};
#define TERICHDB_REGISTER_SEGMENT(SegmentClass, ...) \
	static ReadableSegment::RegisterSegmentFactory \
	regSegment_##SegmentClass({#SegmentClass,##__VA_ARGS__}, \
		[]()->ReadableSegment*{return new SegmentClass();})
	static ReadableSegment*
	createSegment(fstring segmentClass, PathRef segDir, SchemaConfig*);

	ReadableSegment();
	~ReadableSegment();
	virtual class ColgroupSegment* getColgroupSegment() const;
	virtual class ReadonlySegment* getReadonlySegment() const;
	virtual class WritableSegment* getWritableSegment() const;
	inline  class ColgroupSegment* getMergableSegment() const {
		return m_isFreezed ? getColgroupSegment() : nullptr;
	}
	virtual class PlainWritableSegment* getPlainWritableSegment() const;
	virtual llong totalStorageSize() const = 0;
	virtual llong numDataRows() const override final;

	// Index can use different implementation for different
	// index schema and index content features
	virtual ReadableIndex* openIndex(const Schema&, PathRef path) const = 0;

	///@ if segDir==m_segDir, it is a flush
	virtual void loadRecordStore(PathRef segDir) = 0;
	virtual void saveRecordStore(PathRef segDir) const = 0;

	inline  void indexSearchExact(size_t mySegIdx, size_t indexId,
								  fstring key, valvec<llong>* recIdvec,
								  DbContext* ctx) const {
		recIdvec->erase_all();
		indexSearchExactAppend(mySegIdx, indexId, key, recIdvec, ctx);
	}

	virtual void indexSearchExactAppend(size_t mySegIdx, size_t indexId,
										fstring key, valvec<llong>* recIdvec,
										DbContext*) const = 0;

	virtual void selectColumns(llong recId, const size_t* colsId, size_t colsNum,
							   valvec<byte>* colsData, DbContext*) const = 0;
	virtual void selectOneColumn(llong recId, size_t columnId,
								 valvec<byte>* colsData, DbContext*) const = 0;

	virtual void selectColgroups(llong id, const size_t* cgIdvec, size_t cgIdvecSize,
								 valvec<byte>* cgDataVec, DbContext*) const = 0;

	void openIndices(PathRef dir);
	void saveIndices(PathRef dir) const;
	llong totalIndexSize() const;

	void saveIsDel(PathRef segDir) const;
	void loadIsDel(PathRef segDir);
	byte*loadIsDel_aux(PathRef segDir, febitvec& isDel) const;
	void closeIsDel();

	void deleteSegment();

	void load(PathRef segDir) override;
	void save(PathRef segDir) const override;

	size_t getPhysicRows() const;
	size_t getPhysicId(size_t logicId) const;
	size_t getLogicId(size_t physicId) const;

	void addtoUpdateList(size_t logicId);

	bool locked_testIsDel(size_t logicId) const {
		SpinRwLock wsLock(m_segMutex, false);
		return m_isDel[logicId];
	}
    bool testIsDel(size_t logicId) const
    {
        return m_isFreezed ? m_isDel[logicId] : locked_testIsDel(logicId);
    }

	SchemaConfigPtr          m_schema;
	valvec<ReadableIndexPtr> m_indices; // parallel with m_indexSchemaSet
	valvec<ReadableStorePtr> m_colgroups; // indices + pure_colgroups
	size_t      m_delcnt;
	febitvec    m_isDel;
	byte*       m_isDelMmap = nullptr;
	rank_select_se m_isPurged; // just for ReadonlySegment
	byte*          m_isPurgedMmap;
	boost::filesystem::path m_segDir;
	mutable SpinRwMutex m_segMutex;
	valvec<uint32_t> m_updateList; // including deletions
	febitvec    m_updateBits; // if m_updateList is too large, use updateBits
	ReadableStorePtr m_deletionTime; // for snapshot, an uint64 array
	bool        m_tobeDel;
	bool        m_isDirty;
	bool        m_hasLockFreePointSearch;
	bool        m_bookUpdates;
	bool        m_withPurgeBits;  // just for ReadonlySegment
    bool        m_onProcess;
};
typedef boost::intrusive_ptr<ReadableSegment> ReadableSegmentPtr;

class TERICHDB_DLL ColgroupSegment : public ReadableSegment {
public:
	ColgroupSegment();
	~ColgroupSegment();

	ColgroupSegment* getColgroupSegment() const override;

	llong dataInflateSize() const override;
	llong dataStorageSize() const override;
	llong totalStorageSize() const override;

	///@{ Will skip m_isDel'ed records
	StoreIterator* createStoreIterForward(DbContext*) const override;
	StoreIterator* createStoreIterBackward(DbContext*) const override;
	///@}

protected:
	void getValueByPhysicId(size_t id, valvec<byte>* val, DbContext*) const;

	void selectColumnsByPhysicId(llong recId, const size_t* colsId,
				size_t colsNum, valvec<byte>* colsData, DbContext*) const;
	void selectOneColumnByPhysicId(llong recId, size_t columnId,
								valvec<byte>* colsData, DbContext*) const;
	void selectColgroupsByPhysicId(llong id, const size_t* cgIdvec,
			size_t cgIdvecSize, valvec<byte>* cgDataVec, DbContext*) const;

public:

	void saveRecordStore(PathRef segDir) const override;
	void closeFiles();

	friend class DbTable;
	friend class TableIndexIter;
	class MyStoreIterForward;  friend class MyStoreIterForward;
	class MyStoreIterBackward; friend class MyStoreIterBackward;
	llong  m_dataInflateSize;
	llong  m_dataMemSize;
	llong  m_totalStorageSize;
};
typedef boost::intrusive_ptr<ColgroupSegment> ColgroupSegmentPtr;

// Every index is a ReadableIndexStore
//
// The <<store>> is multi-part, because the <<store>> may be
// very large, and the compressible database algo need to fit
// all uncompressed data into memory during compression, split
// the whole data into multi-part reduce the memory usage during
// compression, because just a few part of <<store>> data need
// to fit into memory at a time for compression.
//
// The <<index>> is single-part, because index is much smaller
// than the whole <<store>> data.
//
class TERICHDB_DLL ReadonlySegment : public ColgroupSegment {
public:
	ReadonlySegment();
	~ReadonlySegment();

	ReadonlySegment* getReadonlySegment() const override;

	void convFrom(class DbTable*, size_t segIdx);
	void purgeDeletedRecords(class DbTable*, size_t segIdx);

	void getValueAppend(llong id, valvec<byte>* val, DbContext*) const override;
	void indexSearchExactAppend(size_t mySegIdx, size_t indexId,
								fstring key, valvec<llong>* recIdvec,
								DbContext*) const override;

	void selectColumns(llong recId, const size_t* colsId, size_t colsNum,
					   valvec<byte>* colsData, DbContext*) const override;
	void selectOneColumn(llong recId, size_t columnId,
						 valvec<byte>* colsData, DbContext*) const override;

	void selectColgroups(llong id, const size_t* cgIdvec, size_t cgIdvecSize,
						 valvec<byte>* cgDataVec, DbContext*) const override;

	void load(PathRef segDir) override;
	void save(PathRef segDir) const override;

	virtual ReadableIndex* openIndex(const Schema&, PathRef path) const override = 0;

	virtual ReadableIndex*
			buildIndex(const Schema&, SortableStrVec& indexData)
			const = 0;

	virtual ReadableStore*
			buildStore(const Schema&, SortableStrVec& storeData)
			const = 0;

	virtual ReadableStore*
			buildDictZipStore(const Schema&, PathRef dir, StoreIterator& inputIter,
							  const bm_uint_t* isDel, const febitvec* isPurged)
			const;

	void compressMultipleColgroups(ReadableSegment* input, DbContext* ctx);
	void compressSingleKeyIndex(ReadableSegment* input, DbContext* ctx);
	virtual
	void compressSingleColgroup(ReadableSegment* input, DbContext* ctx);
	virtual
	void compressSingleKeyValue(ReadableSegment* input, DbContext* ctx);

	void completeAndReload(class DbTable*, size_t segIdx,
						   class ReadableSegment* input);
	void syncUpdateRecordNoLock(size_t dstBaseId, size_t logicId,
								const ReadableSegment* input);

	ReadableIndexPtr purgeIndex(size_t indexId, ColgroupSegment* input, DbContext* ctx);
	ReadableStorePtr purgeColgroup(size_t colgroupId, ColgroupSegment* input, DbContext* ctx, PathRef tmpSegDir);
	ReadableStorePtr purgeColgroup_s(size_t colgroupId,
			const febitvec& newIsDel, size_t newDelcnt,
			ColgroupSegment* input, DbContext* ctx, PathRef tmpSegDir);

	void loadRecordStore(PathRef segDir) override;

	void removePurgeBitsForCompactIdspace(PathRef segDir);
	void savePurgeBits(PathRef segDir) const;
};
typedef boost::intrusive_ptr<ReadonlySegment> ReadonlySegmentPtr;

class TERICHDB_DLL DbTransaction : boost::noncopyable {
public:
	enum Status { started, committed, rollbacked } m_status;
	///@{ just for BatchWriter
	//valvec<llong>   m_removeOnCommit;
	//valvec<uint32_t>m_appearOnCommit; // the subId, must be in m_wrSeg
	// @}
    llong m_recId;
	virtual void indexRemove(size_t indexId, fstring key) = 0;
	virtual bool indexInsert(size_t indexId, fstring key) = 0;
	virtual void storeRemove() = 0;
	virtual void storeUpdate(fstring row) = 0;
	virtual void storeGetRow(valvec<byte>* row) = 0;
	void startTransaction(llong recId);
	bool commit();
	void rollback();
	virtual const std::string& strError() const = 0;
	virtual ~DbTransaction();
	DbTransaction();

	virtual void do_startTransaction() = 0;
	virtual bool do_commit() = 0;
	virtual void do_rollback() = 0;
};
class TransactionGuard : boost::noncopyable {
protected:
	DbTransaction* m_txn;
public:
	explicit TransactionGuard(DbTransaction* txn, llong recId) {
		assert(NULL != txn);
		txn->startTransaction(recId);
		m_txn = txn;
	}
	~TransactionGuard() {
		assert(DbTransaction::committed == m_txn->m_status ||
			   DbTransaction::rollbacked == m_txn->m_status);
	}
	DbTransaction* getTxn() const { return m_txn; }
	void indexRemove(size_t indexId, fstring key) {
		m_txn->indexRemove(indexId, key);
	}
	bool indexInsert(size_t indexId, fstring key) {
		return m_txn->indexInsert(indexId, key);
	}
	void storeRemove() {
		m_txn->storeRemove();
	}
	void storeUpdate(fstring row) {
		m_txn->storeUpdate(row);
	}
	void storeGetRow(valvec<byte>* row) {
		m_txn->storeGetRow(row);
	}
	bool commit() {
		assert(DbTransaction::started == m_txn->m_status);
		return m_txn->commit();
	}
	void rollback() {
		assert(DbTransaction::started == m_txn->m_status);
		m_txn->rollback();
	}
	const std::string& strError() const { return m_txn->strError(); }
	const char* szError() const { return m_txn->strError().c_str(); }
};
class TERICHDB_DLL DefaultRollbackTransaction : public TransactionGuard {
public:
	explicit
	DefaultRollbackTransaction(DbTransaction* txn, llong recId) : TransactionGuard(txn, recId) {}
	~DefaultRollbackTransaction();
};

// Concrete WritableSegment should not implement this class,
// should implement PlainWritableSegment or ColgroupWritableSegment
class TERICHDB_DLL WritableSegment : public ColgroupSegment, public WritableStore {
public:
	virtual DbTransaction* createTransaction(DbContext*) = 0;

	WritableSegment();
	~WritableSegment();

	void pushIsDel(bool val);
	void popIsDel();

	ColgroupSegment* getColgroupSegment() const override;
	WritableSegment* getWritableSegment() const override;

	AppendableStore* getAppendableStore() override;
	UpdatableStore* getUpdatableStore() override;
	WritableStore* getWritableStore() override;

	// Index can use different implementation for different
	// index schema and index content features
	virtual ReadableIndex* createIndex(const Schema&, PathRef segDir) const = 0;

	virtual void initEmptySegment() = 0;
	virtual void markFrozen() = 0;

	void indexSearchExactAppend(size_t mySegIdx, size_t indexId,
								fstring key, valvec<llong>* recIdvec,
								DbContext*) const override;

	void flushSegment();

	void delmarkSet0(llong subId);

	valvec<uint32_t>  m_deletedWrIdSet;
};
typedef boost::intrusive_ptr<WritableSegment> WritableSegmentPtr;

class TERICHDB_DLL PlainWritableSegment : public WritableSegment {
	class MyStoreIter;
public:
	ColgroupSegment* getColgroupSegment() const override;
	PlainWritableSegment* getPlainWritableSegment() const override;

	llong totalStorageSize() const override;
	llong dataStorageSize() const override;
	llong dataInflateSize() const override;

	StoreIterator* createStoreIterForward(DbContext*) const override;
	StoreIterator* createStoreIterBackward(DbContext*) const override;

	void initEmptySegment() override;
	void markFrozen() override;

	void getValueAppend(llong recId, valvec<byte>* val, DbContext*) const override;

	llong append(fstring row, DbContext*) override;
	void update(llong id, fstring row, DbContext*) override;
	void remove(llong id, DbContext* ctx) override;
	void shrinkToFit() override;
    void shrinkToSize(size_t size) override;

	void getCombineAppend(llong recId, valvec<byte>* val, valvec<byte>& wrtBuf, ColumnVec& cols1, ColumnVec& cols2) const;

	void selectColumns(llong recId, const size_t* colsId, size_t colsNum,
					   valvec<byte>* colsData, DbContext*) const override;
	void selectOneColumn(llong recId, size_t columnId,
						 valvec<byte>* colsData, DbContext*) const override;

	void selectColumnsByWhole(llong recId,
							  const size_t* colsId, size_t colsNum,
							  valvec<byte>* colsData, DbContext*) const;
	void selectColumnsCombine(llong recId,
							  const size_t* colsId, size_t colsNum,
							  valvec<byte>* colsData, DbContext*) const;

	void selectColgroups(llong id, const size_t* cgIdvec, size_t cgIdvecSize,
						 valvec<byte>* cgDataVec, DbContext*) const override;

	void loadRecordStore(PathRef segDir) override;
	void saveRecordStore(PathRef segDir) const override;

	void getWrtStoreData(llong subId, valvec<byte>* buf, DbContext* ctx) const;

	ReadableStorePtr  m_wrtStore;
};
typedef boost::intrusive_ptr<PlainWritableSegment> PlainWritableSegmentPtr;

// Every index is a WritableIndexStore
// But the <<store>> is not multi-part(such as ReadonlySegment)
class TERICHDB_DLL ColgroupWritableSegment : public WritableSegment {
public:
protected:
	~ColgroupWritableSegment();
	ColgroupSegment* getColgroupSegment() const override;

	virtual ReadableStore* createStore(const Schema&, PathRef segDir) const = 0;
	void initEmptySegment() override;
	void markFrozen() override;

	void getValueAppend(llong id, valvec<byte>* val, DbContext*) const override;

	void indexSearchExactAppend(size_t mySegIdx, size_t indexId,
								fstring key, valvec<llong>* recIdvec,
								DbContext* ctx) const override;

	void loadRecordStore(PathRef segDir) override;
	void saveRecordStore(PathRef segDir) const override;

	llong dataStorageSize() const override;
	llong totalStorageSize() const override;

	void selectColumns(llong recId, const size_t* colsId, size_t colsNum,
					   valvec<byte>* colsData, DbContext*) const override;
	void selectOneColumn(llong recId, size_t columnId,
						 valvec<byte>* colsData, DbContext*) const override;

	void selectColgroups(llong id, const size_t* cgIdvec, size_t cgIdvecSize,
						 valvec<byte>* cgDataVec, DbContext*) const override;
};
typedef boost::intrusive_ptr<ColgroupWritableSegment> SmartWritableSegmentPtr;

} } // namespace terark::terichdb

#endif // __terichdb_segment_hpp__
