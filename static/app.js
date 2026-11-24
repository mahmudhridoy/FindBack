// app.js - helper functions shared by every FindBack page.
// This file uses simple, beginner-friendly JavaScript:
// plain functions, normal for loops, and string joining with +.


// Makes text safe to put inside HTML (so an item name cannot break the page).
function escapeHtml(text) {
  if (text === undefined || text === null) {
    return '';
  }
  text = String(text);
  text = text.replace(/&/g, '&amp;');
  text = text.replace(/</g, '&lt;');
  text = text.replace(/>/g, '&gt;');
  text = text.replace(/"/g, '&quot;');
  text = text.replace(/'/g, '&#39;');
  return text;
}


// Shows a small message at the bottom of the screen.
// Pass true as the second value to show it in red (for errors).
var messageTimer = null;

function showMessage(text, isError) {
  var box = document.getElementById('toast');

  if (box === null) {
    box = document.createElement('div');
    box.id = 'toast';
    box.setAttribute('role', 'status');
    document.body.appendChild(box);
  }

  box.textContent = text;
  if (isError) {
    box.className = 'show bad';
  } else {
    box.className = 'show';
  }

  clearTimeout(messageTimer);
  messageTimer = setTimeout(function () {
    box.className = '';
  }, 3500);
}


// Sends a request to the C++ server.
//   method    : 'GET', 'POST', 'PUT' or 'DELETE'
//   url       : the part after /api, for example '/items'
//   data      : an object to send, or null when there is nothing to send
//   onSuccess : a function that receives the answer from the server
function callApi(method, url, data, onSuccess) {
  var options = {
    method: method,
    headers: { 'Content-Type': 'application/json' }
  };

  if (data !== null) {
    options.body = JSON.stringify(data);
  }

  fetch('/api' + url, options)
    .then(function (response) {
      return response.text().then(function (text) {
        var result = null;
        if (text !== '') {
          result = JSON.parse(text);
        }

        if (response.ok === false) {
          var message = 'Something went wrong (code ' + response.status + ')';
          if (result !== null && result.error) {
            message = result.error;
          }
          showMessage(message, true);
          return;
        }

        onSuccess(result);
      });
    })
    .catch(function (error) {
      console.log(error);
      showMessage('Could not finish that. Is the FindBack server running?', true);
    });
}


// Builds the side menu (computer), the top bar and the bottom tabs (phone).
// activePage is the name of the page being shown, for example 'Dashboard'.
function buildMenu(activePage) {
  var pages = [
    { link: 'index.html', name: 'Dashboard', shortName: 'Home',
      icon: '<path d="M3 11l9-8 9 8v10a1 1 0 0 1-1 1h-5v-6H9v6H4a1 1 0 0 1-1-1z"/>' },
    { link: 'report.html?type=LOST', name: 'Report lost', shortName: 'Lost',
      icon: '<circle cx="11" cy="11" r="7"/><path d="M21 21l-4.3-4.3M8 11h6"/>' },
    { link: 'report.html?type=FOUND', name: 'Report found', shortName: 'Found',
      icon: '<circle cx="12" cy="12" r="9"/><path d="M12 8v8M8 12h8"/>' },
    { link: 'matches.html', name: 'Matches', shortName: 'Matches',
      icon: '<path d="M7 7h11l-3-3M17 17H6l3 3"/>' },
    { link: 'admin.html', name: 'Admin', shortName: 'Admin',
      icon: '<path d="M4 6h10M18 6h2M4 12h4M12 12h8M4 18h12"/><circle cx="16" cy="6" r="2"/><circle cx="10" cy="12" r="2"/><circle cx="18" cy="18" r="2"/>' }
  ];

  var logo = '<div class="logo">Find<span>Back</span></div>';
  var sideLinks = '';
  var tabLinks = '';

  for (var i = 0; i < pages.length; i++) {
    var page = pages[i];
    var activeClass = '';
    if (page.name === activePage) {
      activeClass = 'on';
    }

    var icon = '<svg class="ic" viewBox="0 0 24 24" aria-hidden="true">' + page.icon + '</svg>';

    sideLinks += '<a href="' + page.link + '" class="' + activeClass + '">' + icon + page.name + '</a>';
    tabLinks += '<a href="' + page.link + '" class="' + activeClass + '">' + icon + page.shortName + '</a>';
  }

  var html = '<aside>' + logo + sideLinks + '</aside>';
  html += '<header class="top">' + logo + '</header>';
  html += '<nav class="tabbar" aria-label="Main">' + tabLinks + '</nav>';

  document.body.insertAdjacentHTML('afterbegin', html);
}


// Turns "LOST" into "Lost".
function capitalize(word) {
  if (!word) {
    return '';
  }
  return word.charAt(0).toUpperCase() + word.slice(1).toLowerCase();
}


// Builds the coloured Lost / Found label.
function typeBadge(type) {
  return '<span class="pill ' + String(type).toLowerCase() + '">' + capitalize(type) + '</span>';
}


// Builds the list of items shown on the dashboard and admin pages.
// Pass true as showDelete to add a Delete button to every row.
function makeItemRows(items, showDelete) {
  if (!items || items.length === 0) {
    return '<div class="empty"><b>Nothing here yet</b>Reported items will appear in this list.</div>';
  }

  var html = '<div class="list">';

  for (var i = 0; i < items.length; i++) {
    var item = items[i];

    var letter = item.name;
    if (item.cate) {
      letter = item.cate;
    }
    letter = letter.charAt(0).toUpperCase();

    var statusText = 'Open';
    var statusClass = '';
    if (item.status === 'CLAIMED') {
      statusText = 'Claimed';
      statusClass = ' done';
    }

    html += '<div class="row">';
    html += '<span class="av ' + item.type.toLowerCase() + '">' + escapeHtml(letter) + '</span>';
    html += '<div class="grow">';
    html += '<div class="t">' + escapeHtml(item.name) + ' ' + typeBadge(item.type);
    if (item.urgent) {
      html += ' <span class="pill warn">Urgent</span>';
    }
    html += '</div>';
    html += '<div class="meta">#' + item.id + ' · ' + escapeHtml(item.cate) + ' · ' +
            escapeHtml(item.location) + ' · ' + escapeHtml(item.date) + '</div>';
    html += '</div>';
    html += '<span class="pill' + statusClass + '">' + statusText + '</span>';
    if (showDelete) {
      html += '<button class="btn danger sm" onclick="deleteItem(' + item.id + ')">Delete</button>';
    }
    html += '</div>';
  }

  html += '</div>';
  return html;
}
