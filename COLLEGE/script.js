function showSection(sectionId) {
            const sections = document.querySelectorAll('section');
            sections.forEach(section => {
                section.classList.remove('active');
            });
            document.getElementById(sectionId).classList.add('active');
}

document.addEventListener("keydown", function (event) {
    if (event.key === "PrintScreen") {
        event.preventDefault();
        alert("Screenshots are disabled on this site!");
    }
    if (event.ctrlKey && event.key === "p") {  // Prevent print
        event.preventDefault();
        alert("Printing is disabled!");
    }
});

document.addEventListener("contextmenu", function (event) {
    event.preventDefault();
    alert("Right-click is disabled!");
});
