#include <iostream>
#include <terark/lcast.hpp>
#include <terark/io/DataIO.hpp>

struct Address {
	// demo for CarBinPack
	std::string city; // varintlen + content
	std::string street; // varintlen + content
	DATA_IO_LOAD_SAVE(Address, &city&street)
};
#include "user.hpp"

// g++-4.9 -std=c++1y 3.write_read_delete.cpp -lterichdb-r -lboost_system -lterark-fsa_all-r -lboost_filesystem -Iinclude
// Write data into user_table 
int main(int argc, char* argv[]){
  std::cout<<"Hello Terark, We will write data into db"<<std::endl;

  // open table
  static const char* dbtable = "db";
  terark::terichdb::DbTablePtr tab = test_ns::User::openTable(dbtable);

  // write data (1000 records)
  terark::NativeDataOutput<terark::AutoGrownMemIO> rowBuilder;
  terark::terichdb::DbContextPtr ctx = tab->createDbContext();

  test_ns::User u = {};

  int count = 1000;
  for(int i = 1; i <= count; i++) {
	char szBuf[256];
    u.id = i;
    u.name.assign(szBuf, snprintf(szBuf, sizeof(szBuf), "TestName-%d", i));
    u.description.assign(szBuf, snprintf(szBuf, sizeof(szBuf), "Description-%d", i));
    u.age = (i + 10);
    u.update_time = 1463472964753 + i;
	u.email = "leipeng@terark.com";
	u.city = "beijing";
	u.street = "chaoyang road";
	u.addr.city = u.city;
	u.addr.street = u.street;
    // insert row
    if (ctx->insertRow(u.encode(rowBuilder)) < 0) {
      printf("Insert failed: %s\n", ctx->errMsg.c_str());
    }
  }

  // read data(1000 records)
  terark::valvec<terark::byte> nameVal;     // column value
  terark::valvec<terark::byte> descVal;     // column value
  terark::valvec<terark::byte> nameDescVal;    // column group value
  terark::valvec<terark::byte> rowBuf;    // column group value
  terark::valvec<terark::llong> idvec;      // id vector of index
  terark::terichdb::ColumnVec nameDescCV;

  size_t indexId = tab->getIndexId("id");

  // get data by column id is faster than by column name
  size_t nameColumnId = tab->getColumnId("name");
  size_t descColumnId = tab->getColumnId("description");

  // get a full column group is faster than separately get every column
  size_t nameDescColgroupId = tab->getColgroupId("name_and_description");
  auto& nameDescColgroupSchema = tab->getColgroupSchema(nameDescColgroupId);

  std::cout<<"name colId = "<<nameColumnId<<", desc colId ="<<descColumnId<<std::endl;

  if (indexId >= tab->getIndexNum()) {
    fprintf(stderr, "ERROR: index 'id' does not exist\n");
    terark::terichdb::DbTable::safeStopAndWaitForCompress();
    return 0;
  }

  // iterate ids, read name and description values
  for(int i = 1; i <= count; i++) {
    terark::fstring key = terark::terichdb::Schema::fstringOf(&i);
    ctx->indexSearchExact(indexId, key, &idvec);
    // print ids
    // std::cout<<tab->getIndexSchema(indexId).toJsonStr(key).c_str()<<std::endl;
    for (auto recId : idvec) {
      // the simplest way
      ctx->selectOneColumn(recId, nameColumnId, &nameVal);
      ctx->selectOneColumn(recId, descColumnId, &descVal);
      printf("--%.*s  ", (int)nameVal.size(), nameVal.data());
      printf("%.*s\n", (int)descVal.size(), descVal.data());

	  // name and description are defined in one column group
	  // using selectOneColgroup is much faster
	  ctx->selectOneColgroup(recId, nameDescColgroupId, &nameDescVal);
	  nameDescColgroupSchema.parseRow(nameDescVal, &nameDescCV);
	  terark::fstring name = nameDescCV[0];
	  terark::fstring desc = nameDescCV[1];
// Schema::getNumber has many debug check for type consistency
//	  int assertFail = nameDescColgroupSchema.getNumber<int>(nameDescCV, 0);
      printf("++%.*s  ", name.ilen(), name.data());
      printf("%.*s\n", desc.ilen(), desc.data());

	  // update:
	  ctx->getValue(recId, &rowBuf); // get old value

// old style deserialize:
//    terark::NativeDataInput<terark::MemIO>(rowBuf.range()) >> u;

	  u.decode(rowBuf); // <<< new style deserialize

	  // update columns
	  u.name += " - Add updated suffix ...";
	  u.description += " - add some description ...";

// old style  serialize and update:
//	  rowBuilder.rewind(); // rewind and
//	  rowBuilder << u;     // serialize
//	  ctx->upsertRow(rowBuilder.written());

// new style serialize and update:
	  ctx->upsertRow(u.encode(rowBuilder));
    }
  }

  // delete data(1000 records)
  for(int i = 1; i <= count; i++) {
    terark::fstring key = terark::terichdb::Schema::fstringOf(&i);
    ctx->indexSearchExact(indexId, key, &idvec);
    for (auto recId : idvec) {
      ctx->removeRow(recId);
      // std::cout<<"delete row : "<<recId<<std::endl;
    }
  }

  // close table
  terark::terichdb::DbTable::safeStopAndWaitForCompress();
  return 0;
}
