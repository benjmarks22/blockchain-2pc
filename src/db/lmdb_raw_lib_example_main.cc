// Example binary to test basic functionality of LMDB.
//
// It is a rewriten version of https://github.com/drycpp/lmdbxx/blob/master/example.cc
// Example cmd: 
//   mkdir /tmp/lmdb
//   bazel run lmdb_raw_lib_example_main -- --db_dir=/tmp/lmdb
//
// Example output:
//   ====================Program Starts====================
//   Get within a txn: 'hacker@example.org' 
//   Get commited value via a write txn: 'hacker@example.org' 
//   Readonly txn w/ cursor: 
//   key: 'email', value: 'jhacker@example.org'
//   key: 'fullname', value: 'J. Random Hacker'
//   key: 'username', value: 'jhacker'


#include <cstdio>
#include <cstdlib>

#include "absl/flags/flag.h"
#include "lmdbxx/lmdb++.h"

ABSL_FLAG(std::string, db_dir, "/tmp/lmdb",
          "Database dir for lmdb to temporarily store data."
          "Please create your own dir and pass a absolute path.");

int main() {
  // Create and open the LMDB environment
  std::printf("\n====================Program Starts====================\n");
  lmdb::env env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); // 1 GiB
  env.open(absl::GetFlag(FLAGS_db_dir).c_str(), 0, 0664);

  // Insert some key/value pairs and get tmp value in a write transaction.
  lmdb::txn wtxn = lmdb::txn::begin(env);
  lmdb::dbi dbi = lmdb::dbi::open(wtxn, nullptr);
  dbi.put(wtxn, "username", "jhacker");
  dbi.put(wtxn, "email", "jhacker@example.org");
  dbi.put(wtxn, "fullname", "J. Random Hacker");
  std::string email_val;
  dbi.get(wtxn, "email", email_val);
  std::printf("Get within a txn: '%s' \n", email_val.c_str());
  wtxn.commit();

  // Get commited value of a key via a write transaction.
  lmdb::txn wtxn_get_only = lmdb::txn::begin(env);
  lmdb::dbi dbi_get_only = lmdb::dbi::open(wtxn_get_only, nullptr);
  std::string email_val_get_only;
  dbi_get_only.get(wtxn_get_only, "email", email_val_get_only);
  std::printf("Get commited value via a write txn: '%s' \n", email_val_get_only.c_str());
  wtxn_get_only.commit();

  // Fetch key/value pairs in a read-only transaction
  std::printf("Readonly txn w/ cursor: \n");
  lmdb::txn rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  lmdb::cursor cursor = lmdb::cursor::open(rtxn, dbi);
  std::string key, value;
  while (cursor.get(key, value, MDB_NEXT)) {
    std::printf("key: '%s', value: '%s'\n", key.c_str(), value.c_str());
  }
  cursor.close();
  rtxn.abort();

  // The enviroment is closed automatically.
  return 0;
}