function doGet(e) {

  var cal = CalendarApp.getCalendarsByName('Calendarname')[0]; // 0 is subcalendar ID, mostly "0"
  if (cal == undefined) {
    return ContentService.createTextOutput("no access to calendar");
  }

  const now = new Date();
  var start = new Date(); start.setHours(0, 0, 0);  // start at midnight
  const oneday = 24*3600000; // [msec]
  const stop = new Date(start.getTime() + 14 * oneday); //get appointments for the next 14 days
  
  var events = cal.getEvents(start, stop); //pull start/stop time
  
  var str = '';
  for (var ii = 0; ii < events.length; ii++) {

    var event=events[ii];    
    var myStatus = event.getMyStatus();
    
    
    // define valid entryStatus to populate array
    switch(myStatus) {
      case CalendarApp.GuestStatus.OWNER:
      case CalendarApp.GuestStatus.YES:
      case CalendarApp.GuestStatus.NO:  
      case CalendarApp.GuestStatus.INVITED:
      case CalendarApp.GuestStatus.MAYBE:
      default:
        break;
    }
    
    // Show just every entry regardless of GuestStatus to also get events from shared calendars where you haven't set up the appointment on your own
    str += event.getStartTime() + ';' +
    //event.isAllDayEvent() + '\t' +
    //event.getPopupReminders()[0] + '\t' +
    event.getTitle() +';' + 
    event.isAllDayEvent() + ';';
  }
  return ContentService.createTextOutput(str);
}
