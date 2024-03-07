  
    
      function welcomeUser() {
          var name = document.getElementById("name").value;
          var surname = document.getElementById("surname").value;
          var phone = document.getElementById("phone").value;
          var welcomeMessage = "Welcome " + name + " "+surname+" your phone number is "+phone+"!";
          document.getElementById("welcomeMessage").textContent = welcomeMessage;
      }
      
