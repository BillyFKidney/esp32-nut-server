ObjC.import('Foundation');

const sourcePath = 'src/management-pages.c';
const readError = Ref();
const sourceValue = $.NSString.alloc.initWithContentsOfFileEncodingError(
  $(sourcePath),
  $.NSUTF8StringEncoding,
  readError,
);
if (sourceValue === null) {
  throw new Error(`Unable to read ${sourcePath}.`);
}
const source = ObjC.unwrap(sourceValue);
const allocationMatch = source.match(
  /^#define MANAGEMENT_PAGES_ADMIN_PAGE_LIMIT (\d+)U?$/m,
);
if (allocationMatch === null) {
  throw new Error('Unable to locate the ADMIN page flash-size limit.');
}
const allocationSize = Number(allocationMatch[1]);
const functionStart = source.indexOf('esp_err_t management_pages_send_admin');
const templateStart = source.indexOf('static const char page_template[] =', functionStart);
const lengthCheck = source.indexOf('if (strstr(page_template', templateStart);

if (functionStart < 0 || templateStart < 0 || lengthCheck < 0) {
  throw new Error(`Unable to locate the ADMIN page template in ${sourcePath}.`);
}

const templateSource = source.slice(templateStart, lengthCheck);
const cStrings = templateSource.match(/"(?:\\.|[^"\\\r\n])*"/g) ?? [];
const page = cStrings.map((literal) => eval(literal)).join('').replace(/%%/g, '%');
const csrfPlaceholder = 'x'.repeat(64);
const renderedPage = page.replace('%s', csrfPlaceholder);
if (renderedPage.includes('%s')) {
  throw new Error('ADMIN page has an unexpected additional string placeholder.');
}
const renderedLength = $.NSString.stringWithString($(renderedPage))
  .lengthOfBytesUsingEncoding($.NSUTF8StringEncoding);
if (renderedLength >= allocationSize) {
  throw new Error(
    `ADMIN page renders to ${renderedLength} bytes, exceeding its ${allocationSize}-byte flash-size limit.`,
  );
}
if ((page.match(/%s/g) ?? []).length !== 1) {
  throw new Error('ADMIN page must contain exactly one CSRF placeholder.');
}
const scriptStart = page.indexOf('<script>');
const scriptEnd = page.indexOf('</script>', scriptStart);

if (scriptStart < 0 || scriptEnd < 0) {
  throw new Error('Unable to locate the embedded ADMIN-page script.');
}

new Function(page.slice(scriptStart + '<script>'.length, scriptEnd));
const dashboardStart = page.indexOf('id=panel-dashboard');
const dashboardEnd = page.indexOf('id=panel-logs', dashboardStart);
const dashboardMarkup = page.slice(dashboardStart, dashboardEnd);
const logRouteCount = (page.match(/\/api\/v1\/admin\/logs/g) ?? []).length;
if (dashboardStart < 0 || dashboardEnd < 0 || dashboardMarkup.includes('dashboardLogs')) {
  throw new Error('Dashboard must not contain a runtime-log view.');
}
if (!page.includes('id=panel-logs') || !page.includes('downloadLogsButton') ||
    !page.includes('copyLogsButton') || logRouteCount !== 1) {
  throw new Error('Dedicated Logs page contract is incomplete or duplicated.');
}
if (!page.includes('JSON.stringify(x,null,2)') || !page.includes('lastStatusText')) {
  throw new Error('Device Status must pretty-print JSON while retaining the raw response.');
}
if (!page.includes('class=app-header') || !page.includes('id=headerStatus') ||
    !page.includes('onclick=logout()')) {
  throw new Error('Persistent appliance header controls are incomplete.');
}
if (page.includes('loadStatus();loadLogs();')) {
  throw new Error('Retained logs must load lazily, not during initial page setup.');
}
const output = $.NSString.stringWithString(
  `Validated embedded ADMIN-page JavaScript and ${renderedLength}-byte streamed page within ${allocationSize} bytes from ${sourcePath}.\n`,
);
$.NSFileHandle.fileHandleWithStandardOutput.writeData(
  output.dataUsingEncoding($.NSUTF8StringEncoding),
);
