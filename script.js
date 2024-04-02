  
    function welcomeUser() {
      var traintype = document.getElementById("traintype").value;
      var origin = document.getElementById("origin").value;
      var destination = document.getElementById("destination").value;
      var date = document.getElementById("date").value;
      var welcomeMessage = "Our dear customer, note that you selected " + traintype + " train departing from " + origin + " traveling to " + destination + " and your date of travel is " + date + ". Thank you for engaging with us.";
      document.getElementById("welcomeMessage").textContent = welcomeMessage;
    }
