/*
 * db_context.cpp
 *
 *  Created on: 2015��11��26��
 *      Author: leipeng
 */
#include "db_context.hpp"
#include "db_table.hpp"

namespace nark { namespace db {

DbContext::DbContext(const CompositeTable* tab)
  : m_tab(const_cast<CompositeTable*>(tab))
{
}

DbContext::~DbContext() {
}

} } // namespace nark::db
