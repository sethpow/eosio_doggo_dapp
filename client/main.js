ScatterJS.plugins( new ScatterEOS() );

const network = ScatterJS.Network.fromJson({
    blockchain:'eos',
    chainId:'8a34ec7df1b8cd06ff4a8abbaa7cc50300823350cadc59ab296cb00d104d2b8f',
    host:'127.0.0.1',
    port:8888,
    protocol:'http'
});

const contractConfig = {
    code: "doggo2",
    scope: "doggo2",
    dogTableName: "dogs",
    balancesTableName: "balances",
    symbol: "DOGCOIN"
}

var eos;        // object; used to create actions & transactions to interact with blockchain
var rpc;        // used to fetch info from blockchain; initialize once signed in
var account;

// user sees 'DogDapp' when giving permission(s) to access keys
ScatterJS.connect('DogDapp', {network}).then(connected => {
    if(!connected) return alert("Scatter not detected");
    console.log("Scatter connected...");

    // security measures
    // Scatter obj we will use
    const scatter = ScatterJS.scatter;
    // does not allow user to access Scatter thru window
    window.ScatterJS = null;

    // give permission to webpage to use accounts/keys
    // login; happens here, bc need to be connected to login
        // json arg obj; accounts obj with network array;   on which network do we want to get accounts
        // execute anon fn when promise returns
    scatter.login({ accounts: [network] }).then(function(){
        account = scatter.account('eos');
        
        // used to fetch info from blockchain
        // initialize rpc                   forms full url
        rpc = new eosjs_jsonrpc.JsonRpc(network.fullhost());

        // object; used to create actions & transactions to interact with blockchain
            // initializes eos obj that is used to create transactions
        eos = scatter.eos(network, eosjs_api.Api, { rpc });

        getDogs();
        getBalances();
    });
});

function getDogs(){
    // takes 1 arg; args used in cleos to fetch table data
    rpc.get_table_rows({
        json: true,                             // response in json
        code: contractConfig.code,              // acc where table is deployed
        scope: contractConfig.scope,            // scope
        table: contractConfig.dogTableName,     // end here if only want entire table
        index_position: 2,                      // filter rows where current acc is the owner
        key_type: "name",
        lower_bound: account.name,              // actual filter; comes from scatter
        upper_bound: account.name
    }).then(function( res ){
        populateDogList( res.rows )
    })
}

function populateDogList( dogs ){
    // empty out list if already had contents
    $("#doglist").empty()

    var ul = document.getElementById("doglist")
    for (let i = 0; i < dogs.length; i++) {
        var li = document.createElement('li')
        li.appendChild(document.createTextNode(dogs[i].id + ": " + dogs[i].dog_name + ", " + dogs[i].age))
        ul.appendChild(li)
    }
}

function getBalances(){
    rpc.get_table_rows({
        json: true,
        code: contractConfig.code,
        scope: account.name,
        table: contractConfig.balancesTableName
    }).then(function(res){
        populateBalanceList(res.rows)
    })
}

function populateBalanceList( balances ){
    // empty out list if already had contents
    $("#balance_list").empty()

    var ul = document.getElementById("balance_list")
    for (let i = 0; i < balances.length; i++) {
        var li = document.createElement('li')
        li.appendChild(document.createTextNode( balances[i].funds ))
        ul.appendChild(li)
    }
}

// add dog; sending an action containing a transaction to blockchain to call SC
function addDog(){
    // grab var's user enters
    var dogName = $("#dog_name").val()
    var dogAge = $("#dog_age").val()

    // create actual transaction
    // takes 2 args; 2 json obj's; 1 - details about tx and its contents, 2 - TOPOS; formality added for EOS to accept tx
    eos.transact({
        // ARG 1
        // array - can take multiple args
        actions: [{
            account: contractConfig.code,
            name: "insert",     // action(s) this tx should contain
            authorization: [{   // this all comes from scatter
                actor: account.name,
                permission: account.authority
            }],
            // data to send
            data: {
                owner: account.name,
                dog_name: dogName,
                age: dogAge
            }
        }]
    },{     // ARG 2
        // reference to prev block
        blocksBehind: 3,
        // expire time; how long tx is allowed to run
        expireSeconds: 30
    }).then(function( res ){
        // tx is successful
        // refresh list; query blockchain for new dogs & update list
        getDogs()
        // refresh balance; query blockchain for updated balance
        getBalances()
    }).catch(function( err ){
        alert('Error: ', err)
    })
}

function removeDog(){
    var dogId= $("#dog_id").val()

    eos.transact({
        actions: [{
            account: contractConfig.code,
            name: "erase",
            authorization: [{
                actor: account.name,
                permission: account.authority
            }],
            data: {
                dog_id: dogId
            }
        }]
    },{
        blocksBehind: 3,
        expireSeconds: 30
    }).then(function(res){
        getDogs()
    }).catch(function(err){
        alert('Error: ', err)
    })
}

function removeAll(){
    eos.transact({
        actions: [{
            account: contractConfig.code,
            name: "removeall",
            authorization: [{
                actor: account.name,
                permission: account.authority
            }],
            data: {
                owner: account.name
            }
        }]
    },{
        blocksBehind: 3,
        expireSeconds: 30
    }).then(function(){
        getDogs()
    }).catch(function(err){
        alert("Error: ", err)
    })
}

function addToBalance(){
    let amount = $("#balanceAmount").val()
    let memo = $("#memo").val()
    let asset = amount + " " + contractConfig.symbol

    eos.transact({
        actions: [
            {
                account: 'eosio.token',
                name: 'transfer',
                authorization: [
                    {
                        actor: account.name,
                        permission: account.authority
                    }
                ],
                data: {
                    from: account.name,
                    to: contractConfig.code,
                    quantity: asset,
                    memo: memo
                }
            }
        ]
    },{
        blocksBehind: 3,
        expireSeconds: 30
    }).then(function(){
        getBalances()
    }).catch(function(err){
        alert('Error: ', err)
    })
}


// wait til html page has loaded, and then execute code; or else button wont be loaded, and id isnt there
$(document).ready(function() {
    // add_dog_button listener
    $("#add_dog_button").click(addDog)
    $("#remove_dog_button").click(removeDog)
    $("#remove_all_button").click(removeAll)
    $("#add_to_balance").click(addToBalance)
});
