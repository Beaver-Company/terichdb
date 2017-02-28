#pragma once

#include <terark/terichdb/db_segment.hpp>
#include <wiredtiger.h>

namespace terark { namespace terichdb { namespace wt {

class TERICHDB_DLL WtWritableSegment : public PlainWritableSegment {
public:
	class WtDbTransaction; friend class WtDbTransaction;
	DbTransaction* createTransaction(DbContext*) override;

	WtWritableSegment();
	~WtWritableSegment();

protected:
	void init(PathRef segDir);

	ReadableIndex* createIndex(const Schema&, PathRef segDir) const override;
	ReadableIndex* openIndex(const Schema&, PathRef segDir) const override;

	void initEmptySegment() override;
	void load(PathRef path) override;
	void save(PathRef path) const override;

	WT_CONNECTION* m_wtConn;
	WritableStore* m_wrRowStore;
	size_t m_cacheSize;
};

}}} // namespace terark::terichdb::wt
