

function validateUserSignIn(){
    const isLogged = localStorage.getItem("login");
    return isLogged ? true : false;
}

function signIn( username, password ){
    const json = {
        "user":username,
        "password":password,
    }
    fetch("http://127.0.0.1:2001/login", {
                method:"POST",
                mode:'cors',
                headers: {
                    "Content-Type":"application/json"
                },
                body: JSON.stringify( json )
    })
    .then(response => {
        if(response.ok){
            localStorage.setItem("login", "true");
            redirect("./index.html");
        }
        else
            throw Error("Error in signin fetch");
    })
    .catch(error=>{
        alert(error);
    });
}

function signUp( username, password ){
    const json = {
        "user":username,
        "password":password,
    }
    fetch("http://127.0.0.1:2001/register", {
                method:"POST",
                mode:'cors',
                headers: {
                    "Content-Type":"application/json"
                },
                body: JSON.stringify( json )
    })
    .then(response => {
        if(response.ok){
            localStorage.setItem("login", "true");
            redirect("./index.html");
        }
        else
            throw Error("Error in signin fetch");
    })
    .catch(error=>{
        alert(error);
    });
}

function redirect(url){
    document.location.assign(url);
}



function loadPokemonInformation(pokemonId){
    fetch( ("http://127.0.0.1:2001/pokemon/" + pokemonId), {
        method:"GET",
        mode:'cors'
    })
    .then( async response => {
        if(response.ok){
            const res = await response.json();
            localStorage.setItem( "pokemonInformation", JSON.stringify(res) );
            redirect("./pokemon.html");
        }
        else
            throw Error("No estas registrado");
    })
    .catch(error=>{
        alert(error);
    });
}

function createPokemonNameBox(pokemonName, id){
    const box = document.createElement("button");
    box.addEventListener("click", ()=>{ 
        if( validateUserSignIn() ){
            loadPokemonInformation(id);
        }
        else{
            alert("Necesitas estar registrado");
            redirect("./login.html");
        }

    });
    const name = document.createElement("h3");
    name.innerText = pokemonName;
    box.appendChild(name);
    return box;
}

function LoadPokemonNames(offset=0){
    
    const pokemonList = document.getElementById("pokemon-list");
    pokemonList.innerHTML = "";
    fetch("http://127.0.0.1:2001/?offset="+offset)
    .then( async response => {
        if(response.ok){
            const json = await response.json();
            localStorage.setItem("pokemonCount", json.count);
            localStorage.setItem("pokemonIndex", offset);

            let counter = Number(offset)+1;
            json.pokemons.map( pokemon => {
                pokemonList.appendChild( createPokemonNameBox( pokemon.name, counter ) );
                counter++;        
            } );
        }
        else
            throw Error("Error");
    })
    .catch( error => {
        alert( error );
    });
}

