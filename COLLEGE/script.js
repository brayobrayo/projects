//display section based on the button clicked in navigation bar
function showSection(sectionId) {
    const sections = document.querySelectorAll('section');
    sections.forEach(section => section.classList.remove('active'));
    document.getElementById(sectionId).classList.add('active');
  }

//display submenu from an item in main menu  
function toggleSubmenu(item) {
    item.classList.toggle("expanded");
  }
//display sidebar menu from three bar button 
  function toggleSidebar(icon) {
    icon.classList.toggle("active");
    const sidebar = document.getElementById("mySidebar");
    if (sidebar.style.width === "250px") {
        sidebar.style.width = "0";
      } else {
        sidebar.style.width = "250px";}
  

  // Disable screenshot/print
  document.addEventListener("keydown", function (event) {
    if (event.key === "PrintScreen" || (event.ctrlKey && event.key === "p")) {
      event.preventDefault();
      alert("Screenshots and printing are disabled!");
    }
  });
  document.addEventListener("contextmenu", function (event) {
    event.preventDefault();
    alert("Right-click is disabled!");
  });}
