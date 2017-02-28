#ifndef __terichdb_mock_db_index_hpp__
#define __terichdb_mock_db_index_hpp__

#include <terark/terichdb/db_table.hpp>
#include <terark/terichdb/db_segment.hpp>
#include <terark/util/fstrvec.hpp>
#include <set>
#include <mutex>

namespace terark { namespace terichdb {

class TERICHDB_DLL MockReadonlyStore : public ReadableStore {
	size_t    m_fixedLen;
	const Schema& m_schema;
public:
	fstrvec m_rows;
	explicit MockReadonlyStore(const Schema&);

	void build(const Schema&, SortableStrVec& storeData);

	void save(PathRef) const override;
	void load(PathRef) override;

	llong dataStorageSize() const override;
	llong dataInflateSize() const override;
	llong numDataRows() const override;
	void getValueAppend(llong id, valvec<byte>* val, DbContext*) const;
	StoreIterator* createStoreIterForward(DbContext*) const override;
	StoreIterator* createStoreIterBackward(DbContext*) const override;
};
typedef boost::intrusive_ptr<MockReadonlyStore> MockReadonlyStorePtr;

class TERICHDB_DLL MockReadonlyIndex : public ReadableIndex, public ReadableStore {
	friend class MockReadonlyIndexIterator;
	friend class MockReadonlyIndexIterBackward;
	fstrvec          m_keys; // keys[recId] is the key
	valvec<uint32_t> m_ids;  // keys[ids[i]] <= keys[ids[i+1]]
	size_t m_fixedLen;
	const Schema* m_schema;
	void getIndexKey(llong* id, valvec<byte>* key, size_t pos) const;
	int forwardLowerBound(fstring key, size_t* pLower) const;
public:
	MockReadonlyIndex(const Schema& schema);
	~MockReadonlyIndex();

	void build(SortableStrVec& indexData);

	void save(PathRef) const override;
	void load(PathRef) override;

	StoreIterator* createStoreIterForward(DbContext*) const override;
	StoreIterator* createStoreIterBackward(DbContext*) const override;
	llong numDataRows() const override;
	llong dataStorageSize() const override;
	llong dataInflateSize() const override;
	void getValueAppend(llong id, valvec<byte>* key, DbContext*) const override;

	void searchExactAppend(fstring key, valvec<llong>* recIdvec, DbContext*) const override;

	IndexIterator* createIndexIterForward(DbContext*) const override;
	IndexIterator* createIndexIterBackward(DbContext*) const override;
	llong indexStorageSize() const override;

	ReadableIndex* getReadableIndex() override;
	ReadableStore* getReadableStore() override;
};
typedef boost::intrusive_ptr<MockReadonlyIndex> MockReadonlyIndexPtr;

class TERICHDB_DLL MockWritableStore : public ReadableStore, public WritableStore {
public:
	valvec<valvec<byte> > m_rows;
	llong m_dataSize;
	mutable SpinRwMutex m_rwMutex;

	MockWritableStore();
	~MockWritableStore();

	void save(PathRef) const override;
	void load(PathRef) override;

	llong dataStorageSize() const override;
	llong dataInflateSize() const override;
	llong numDataRows() const override;
	void getValueAppend(llong id, valvec<byte>* val, DbContext*) const override;
	StoreIterator* createStoreIterForward(DbContext*) const override;
	StoreIterator* createStoreIterBackward(DbContext*) const override;

	llong append(fstring row, DbContext*) override;
	void  update(llong id, fstring row, DbContext*) override;
	void  remove(llong id, DbContext*) override;

	void shrinkToFit() override;
    void shrinkToSize(size_t size) override;

	AppendableStore* getAppendableStore() override;
	UpdatableStore* getUpdatableStore() override;
	WritableStore* getWritableStore() override;
};
typedef boost::intrusive_ptr<MockWritableStore> MockWritableStorePtr;

template<class Key>
class TERICHDB_DLL MockWritableIndex : public ReadableIndex, public WritableIndex {
	class MyIndexIterForward;  friend class MyIndexIterForward;
	class MyIndexIterBackward; friend class MyIndexIterBackward;
	typedef std::pair<Key, llong> kv_t;
	std::set<kv_t> m_kv;
	size_t m_keysLen;
	mutable SpinRwMutex m_rwMutex;
public:
	explicit MockWritableIndex(bool isUnique);
	void save(PathRef) const override;
	void load(PathRef) override;

	IndexIterator* createIndexIterForward(DbContext*) const override;
	IndexIterator* createIndexIterBackward(DbContext*) const override;
	llong indexStorageSize() const override;
	bool remove(fstring key, llong id, DbContext*) override;
	bool insert(fstring key, llong id, DbContext*) override;
	bool replace(fstring key, llong oldId, llong newId, DbContext*) override;
	void clear() override;

	void searchExactAppend(fstring key, valvec<llong>* recIdvec, DbContext*) const override;
	WritableIndex* getWritableIndex() override { return this; }
};

class TERICHDB_DLL MockReadonlySegment : public ReadonlySegment {
public:
	MockReadonlySegment();
	~MockReadonlySegment();
protected:
	ReadableIndex* openIndex(const Schema&, PathRef path) const override;

	ReadableIndex* buildIndex(const Schema&, SortableStrVec& indexData) const override;
	ReadableStore* buildStore(const Schema&, SortableStrVec& storeData) const override;
	ReadableStore* buildDictZipStore(const Schema&, PathRef, StoreIterator& iter,
					  const bm_uint_t* isDel, const febitvec* isPurged) const override;
};

class TERICHDB_DLL MockWritableSegment : public PlainWritableSegment {
public:
	MockWritableSegment(PathRef dir);
	MockWritableSegment();
	~MockWritableSegment();
	std::mutex          m_mockTxnMutex;
protected:
	DbTransaction* createTransaction(DbContext*);
	ReadableIndex* createIndex(const Schema&, PathRef path) const override;
	ReadableIndex* openIndex(const Schema&, PathRef) const override;
};

} } // namespace terark::terichdb

#endif // __terichdb_mock_db_index_hpp__
