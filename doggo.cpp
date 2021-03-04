#include <eosio/eosio.hpp>
#include <eosio/contract.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

CONTRACT doggo : public contract
{
public:
    using contract::contract;

    doggo(name receiver, name code, datastream<const char*> ds):
        contract(receiver, code, ds),
        currency_symbol("DOGCOIN", 0)
        {  }


    ACTION insert(name owner, std::string dog_name, int age)
    {
        require_auth( owner );

        // check balance of sender/owner
        check_balance( owner );

        // reduce balance
        reduce_balance( owner );

        dog_index dogs(get_first_receiver(), get_first_receiver().value);
        dogs.emplace(owner, [&]( auto& row ) {
            row.id = dogs.available_primary_key ();
            row.dog_name = dog_name;
            row.age = age;
            row.owner = owner;
        });
        send_summary(owner, "inserted dog");
    }

    ACTION erase(int dog_id) 
    {
        dog_index dogs( get_self(), get_self().value);

        auto dog = dogs.get(dog_id, "Unable to find dog");
        require_auth(dog.owner);

        auto iterator = dogs.find(dog_id);
        dogs.erase(iterator);
        send_summary(dog.owner, "erased dog");
    }

    ACTION removeall(name owner)
    {
        dog_index dogs(get_first_receiver(), get_first_receiver().value);

        auto owner_index = dogs.get_index<"byowner"_n>();
        auto iterator = owner_index.find(owner.value);

        while(iterator != owner_index.end()){
            owner_index.erase(iterator);
            iterator = owner_index.find(owner.value);
        }
        send_summary(owner, "erased all his/her dogs");
    }

    // /*
    ACTION notify(name owner, std::string msg)
    {
        // auth of actual contract; this action should only be able to be called from within contract itself
            // inline action call - call from within this contract to notify
            // in order for this to actually be an action that can be executed like that, we need to req the auth of the contract itself
        require_auth(get_self());
        
        // owner is person making the original fn call to insert, erase, etc...
            // need require_recipient in order to notify the user/owner that this inline action was done
            // sends receipt to original caller; no "unknown" inline actions are being called w/o them knowing
        require_recipient(owner);
    }
    // */

    // pay fn - triggered by ACTION; not an inline action
        // listen for transfer that are coming to us; listen for action in another contract
    [[eosio::on_notify("eosio.token::transfer")]]
    void pay( name from, name to, asset quantity, std::string memo ) // asset returns quantity & symbol
    {
        // filter out events we are not interested in
        if(from == get_self() || to != get_self())
        { // do not want if: from me, or not to me
            return;
        }

        // transfer from someone else, coming to us
        check(quantity.amount > 0, "Not enough coins");
        // receive our currency
        check(quantity.symbol == currency_symbol, "Incorrect coin/token");

        // increase balance in balance table
            // get_self() - get table in this contract
            // from.value - individually scoped table; to get your balance, call with your individual scope
                // gives us sender of tx, where we want to increase balance
        balance_index balances(get_self(), from.value);

        // returns row (same as primary key)
            // every scope of table only has 1 row; our currency; find iterator by taking int format of currency_symbol, which gives us the correct row
        auto iterator = balances.find(currency_symbol.raw());

        // table already has a row with balance; need to update amount
        if(iterator != balances.end()) {
            // modify at iterator position; get_self() pays for storage; what to do to that balance
            balances.modify(iterator, get_self(), [&](auto& row){
                row.funds += quantity;
            });
        } else {
            // no row entry with balance
            // contract pay for storage; what to do with balance
            balances.emplace(get_self(), [&](auto& row){
                row.funds = quantity;
            });
        }
    }

private:
    const symbol currency_symbol;

    TABLE dog
    {
        int id;
        std::string dog_name;
        int age;
        name owner;

        uint64_t primary_key() const { return id; }
        uint64_t get_secondary_1() const { return owner.value; }
    };

    // /*
    TABLE balance
    {
        asset funds;

        uint64_t primary_key() const { return funds.symbol.raw(); }
    };
    // */


    /*  args 
            permission_level,
            code,
            action,
            data
    */
    // /*
    void send_summary(name owner, std::string message){
        action(
            // permission_level{get_self(),"active"_n},
            permission_level{get_self(),"active"_n},
            get_self(),
            "notify"_n,
            // std::make_tuple(owner, name{owner}.to_string() + message)
            std::make_tuple(owner, message)
        ).send();
    };
    // */

    typedef multi_index<"dogs"_n, dog, indexed_by<"byowner"_n, const_mem_fun<dog, uint64_t, &dog::get_secondary_1>>> dog_index;

    typedef multi_index<"balances"_n, balance> balance_index;

    // check owner balance
    void check_balance( name owner )
    {               //          account     scope
        balance_index balances( get_self(), owner.value );

        // get row with currency on it; error msg   -   get brings the row
        auto row = balances.get( currency_symbol.raw(), "No DOGCOIN balance" );

        // need 10 DC's to insert dog
        check(row.funds.amount >= 10, "Not enough DOGCOIN");
    }

    // reduce balance
    void reduce_balance( name owner )
    {               //          account     scope
        balance_index balances( get_self(), owner.value );
        
        // need to modify row, so we use iterator & find    -   find brings iterator
        auto iterator = balances.find( currency_symbol.raw() );

        if(iterator != balances.end()){
            balances.modify( iterator, get_self(), [&](auto& row){
                row.funds.set_amount( row.funds.amount - 10 );
            });
        }
    }

};