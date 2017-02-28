#pragma once

#include <terark/terichdb/db_index.hpp>
#include <terark/int_vector.hpp>
#include <terark/rank_select.hpp>
#include <terark/util/sortable_strvec.hpp>
#include <terark/terichdb/db_segment.hpp>

namespace terark { namespace terichdb {

class TERICHDB_DLL FixedLenStore : public ReadableStore, public WritableStore {
public:
	explicit FixedLenStore(const Schema& schema);
	FixedLenStore(PathRef segDir, const Schema& schema);
	~FixedLenStore();

//	static std::string makeFilePath(PathRef segDir, const Schema& schema);

	llong dataStorageSize() const override;
	llong dataInflateSize() const override;
	llong numDataRows() const override;
	void getValueAppend(llong id, valvec<byte>* val, DbContext*) const override;

	StoreIterator* createStoreIterForward(DbContext*) const override;
	StoreIterator* createStoreIterBackward(DbContext*) const override;

	void build(SortableStrVec& strVec);
	void load(PathRef path) override;
	void save(PathRef path) const override;

	void openStore();

	WritableStore* getWritableStore() override;
	AppendableStore* getAppendableStore() override;
	UpdatableStore* getUpdatableStore() override;

	llong append(fstring row, DbContext*) override;

	void  update(llong id, fstring row, DbContext*) override;

	void remove(llong id, DbContext*) override;
	void shrinkToFit() override;
    void shrinkToSize(size_t size) override;

    void markFrozen() override;

	void setNumRows(size_t rows);
	void reserveRows(size_t rows);

	void deleteFiles() override;

	void unneedsLock() { m_needsLock = false; }

protected:
	struct  Header;
	Header* allocFileSize(ullong size);
	Header* m_mmapBase;
	size_t  m_mmapSize;
	size_t  m_fixlen;
	std::string m_fpath;
	const Schema& m_schema;
	mutable SpinRwMutex m_mutex;
	bool        m_needsLock;

	std::pair<size_t, bool> searchLowerBound(fstring binkey) const;
};
typedef boost::intrusive_ptr<class FixedLenStore> FixedLenStorePtr;

}} // namespace terark::terichdb
