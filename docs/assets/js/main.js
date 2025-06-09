document.addEventListener('DOMContentLoaded', function() {
  // Mobile menu toggle
  const mobileMenuButton = document.querySelector('[aria-controls="mobile-menu"]');
  const mobileMenu = document.getElementById('mobile-menu');
  
  if (mobileMenuButton && mobileMenu) {
    mobileMenuButton.addEventListener('click', function() {
      const isExpanded = this.getAttribute('aria-expanded') === 'true';
      this.setAttribute('aria-expanded', !isExpanded);
      mobileMenu.classList.toggle('hidden');
    });
  }
  
  // Add copy functionality to code blocks
  document.querySelectorAll('pre').forEach(pre => {
    // Skip if already has a copy button
    if (pre.querySelector('.copy-button')) return;
    
    const button = document.createElement('button');
    button.className = 'absolute top-2 right-2 bg-gray-700 text-white text-xs px-2 py-1 rounded';
    button.textContent = 'Copy';
    button.title = 'Copy to clipboard';
    
    button.addEventListener('click', () => {
      const code = pre.querySelector('code')?.innerText || pre.innerText;
      navigator.clipboard.writeText(code).then(() => {
        const originalText = button.textContent;
        button.textContent = 'Copied!';
        button.classList.remove('bg-gray-700');
        button.classList.add('bg-green-600');
        
        setTimeout(() => {
          button.textContent = originalText;
          button.classList.remove('bg-green-600');
          button.classList.add('bg-gray-700');
        }, 2000);
      }).catch(err => {
        console.error('Failed to copy text: ', err);
      });
    });
    
    if (window.getComputedStyle(pre).position === 'static') {
      pre.style.position = 'relative';
    }
    
    pre.appendChild(button);
  });
  
  // Smooth scrolling for anchor links
  document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
      e.preventDefault();
      
      const targetId = this.getAttribute('href');
      if (targetId === '#') return;
      
      const targetElement = document.querySelector(targetId);
      if (targetElement) {
        targetElement.scrollIntoView({
          behavior: 'smooth'
        });
      }
    });
  });
});
