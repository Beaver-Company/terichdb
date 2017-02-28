#pragma once

#include <terark/terichdb/db_table.hpp>
#include <terark/terichdb/db_segment.hpp>
#include <terark/util/fstrvec.hpp>
#include <set>
#include <tbb/spin_rw_mutex.h>
#include <terark/mempool.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <terark/io/var_int.hpp>

namespace terark { namespace terichdb { namespace trbdb {

class TrbStoreIterForward;
class TrbStoreIterBackward;

typedef tbb::spin_rw_mutex TrbStoreRWLock;

class TERICHDB_DLL TrbWritableStore : public ReadableStore, public WritableStore {
protected:
    typedef std::size_t size_type;
    typedef terark::MemPool<8> pool_type;
    static size_t constexpr index_shift = 3;
    struct data_object
    {
        byte data[1];
    };
    valvec<uint32_t> m_index;
    pool_type m_data;
    size_t m_size;
    mutable TrbStoreRWLock m_rwMutex;

    fstring readItem(size_type i) const;
    void storeItem(size_type i, fstring d);
    void removeItem(size_type i);

    friend class TrbStoreIterForward;
    friend class TrbStoreIterBackward;

public:
    TrbWritableStore(Schema const &);
	~TrbWritableStore();

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
typedef boost::intrusive_ptr<TrbWritableStore> TrbWritableStorePtr;


class TERICHDB_DLL MemoryFixedLenStore : public ReadableStore, public WritableStore
{
protected:
    size_t m_fixlen;
    valvec<byte> m_data;
    mutable TrbStoreRWLock m_rwMutex;

public:
    MemoryFixedLenStore(Schema const &);
    ~MemoryFixedLenStore();

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
typedef boost::intrusive_ptr<MemoryFixedLenStore> MemoryFixedLenStorePtr;

}}} // namespace terark::terichdb::wt
