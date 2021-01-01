// 2018.07.01  Collaboration: WendyH, Big Dog, михаил, Spell
///////////////////////  Создание структуры подкаста  /////////////////////////

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string gsUrlBase = ''; // Url база ссылок нашего сайта (берётся из корневого элемента)
TDateTime gTimeStart=Now;
int gnTotalItems=0;
///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Установка переменной Podcast: поиск родительской папки, содержащий скрипт
THmsScriptMediaItem GetRoot() {
  Podcast = FolderItem; // Начиная с текущего элемента, ищется создержащий срипт
  while ((Trim(Podcast[550])=='') && (Podcast.ItemParent!=nil)) Podcast=Podcast.ItemParent;
  return Podcast;
}

///////////////////////////////////////////////////////////////////////////////
// Создание подкаста
THmsScriptMediaItem CreatePodcast(THmsScriptMediaItem Folder, string sName, string sLink, String sParams='') {
  THmsScriptMediaItem Item;                // Объявляем переменные
//sLink = HmsExpandLink(sLink, gsUrlBase); // Делаем ссылку полной, если она таковой не является
  Item  = Folder.AddFolder(sLink);         // Создаём подкаст с указанной ссылкой
  Item[mpiTitle] = sName;                  // Присваиваем наименование
  Item[mpiPodcastParameters] = sParams;    // Дополнительные параметры подкаста
  return Item;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Функция создания динамической папки с указанным скриптом
THmsScriptMediaItem CreateDynamicItem(THmsScriptMediaItem prntItem, char sTitle, char sLink, char &sScript='') {
  THmsScriptMediaItem Folder = prntItem.AddFolder(sLink, false, 32);
  Folder[mpiTitle     ] = sTitle;
  Folder[mpiCreateDate] = VarToStr(IncTime(Now,0,-prntItem.ChildCount,0,0));
  Folder[200] = 5;           // mpiFolderType
  Folder[500] = sScript;     // mpiDynamicScript
  Folder[501] = 'JScript'; // mpiDynamicSyntaxType
  Folder[mpiFolderSortOrder] = -mpiCreateDate;
  return Folder;
}

///////////////////////////////////////////////////////////////////////////////
// Удаление всех существующих разделов(перед созданием) кроме поиска
void DeleteFolders() {
  THmsScriptMediaItem Item, FavFolder; int i, nAnsw;
  for (i=FolderItem.ChildCount-1; i>=0; i--) {
    Item = FolderItem.ChildItems[i]; if (Item[mpiFilePath]=='-SearchFolder') continue;
    Item.Delete();
  }
}


///////////////////////////////////////////////////////////////////////////////
// Создание папки ПОИСК (с загрузкой скрипта с форума homemediaserver.ru)
void CreateSearchFolder(THmsScriptMediaItem prntItem, char sTitle) {
  char sScript='', sLink, sHtml, sRE, sVal; THmsScriptMediaItem Folder;
  
  // Да да, загружаем скрипт с сайта форума HMS
 ///////////////////////////////////////////////////////////////////////////////
// Создание папки ПОИСК (с загрузкой скрипта с форума homemediaserver.ru)

  // Да да, загружаем скрипт с сайта форума HMS
  sHtml = HmsUtf8Decode(HmsDownloadURL('http://homemediaserver.ru/forum/viewtopic.php?f=15&t=2793&p=17395'));
  HmsRegExMatch('BeginSearchJScript\\*/(.*?)/\\*EndSearchJScript', sHtml, sScript, 1, PCRE_SINGLELINE);
  sScript = HmsHtmlToText(sScript, 1251);
  sScript = ReplaceStr(sScript, #160, ' ');

  Folder = prntItem.AddFolder(sTitle, true);
  Folder[mpiCreateDate     ] = VarToStr(IncTime(gTimeStart,0,-gnTotalItems,0,0));
  Folder[mpiFolderSortOrder] = "-mpCreateDate";
  gnTotalItems++;
  
  CreateDynamicItem(Folder, '"Набрать текст"', '-SearchCommands', sScript);
}



///////////////////////////////////////////////////////////////////////////////
// Создание подкаста или папки
THmsScriptMediaItem CreateItem(THmsScriptMediaItem Parent, string sTitle="", string sLink="", string sImg="") {
  THmsScriptMediaItem Item; bool bForceFolder = false;
  
  if (sLink=='') { sLink = sTitle; bForceFolder = true; }
  else             sLink = HmsExpandLink(sLink, gsUrlBase);
  
  Item = Parent.AddFolder(sLink, bForceFolder);
  Item[mpiTitle     ] = sTitle;
  Item[mpiCreateDate] = VarToStr(IncTime(gTimeStart,0,-gnTotalItems,0,0));
  Item[mpiFolderSortOrder] = -mpiCreateDate;
  Item[mpiThumbnail ] = sImg;
  gnTotalItems++;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Создание структуры
void CreateStructure() {
  string sHtml, sData,sPro, sName, sLink; TRegExpr RegEx, RegExs;         // Объявляем переменные
  THmsScriptMediaItem Folder, Item;
 
  CreateSearchFolder (FolderItem, '0. Поиск');
 // Folder = CreateItem(FolderItem, '1. Избранное');
  CreatePodcast(FolderItem, '1. Последние поступления', '/'); 
  CreatePodcast(FolderItem, '2 Популярные фильмы'    , '/popular/films'); 
  CreatePodcast(FolderItem, '3. Мультфильмы'          , '/multfilms/');
  CreatePodcast(FolderItem, '4. Мультсериалы'         , '/multserialy/', '--pages=10');
  sHtml = HmsUtf8Decode(HmsDownloadUrl(gsUrlBase));  // Загружаем страницу
  sHtml = HmsRemoveLineBreaks(sHtml);                // Удаляем переносы строк
  HmsRegExMatch('var user_data\\s+=\\s\\{.*is_user_pro_plus:\\s(\\d+)\\};', sHtml, sPro); // Проверка аккаунта Pro +
  CreatePodcast(FolderItem, '5. Сериалы'              , '/serials/');
  if(sPro=='1'){
    CreatePodcast(FolderItem, '5а. 4k'                , '/serials/q4/');
    CreatePodcast(FolderItem, '5b. 2k'                , '/serials/q2/');
    CreatePodcast(FolderItem, '5c. 1080'              , '/serials/qh/');  
  }
  //Folder[mpiPodcastParameters] = '--maxpages=20';
  Folder = FolderItem.AddFolder('6. По категориям сериалы', true);    // Создаём папку
  HmsRegExMatch('menu-title">Сериалы<.*?</ul>(.*?)class="lucky"', sHtml, sData);
  
  
  // Создаём объект для поиска текста и ищем в цикле по регулярному выражению
  RegExs = TRegExpr.Create('<a[^>]+href="(.*?)".*?</a>'); 
  try {
    if (RegExs.Search(sData)) do {     // Если нашли совпадение, запускаем цикл 
      sLink = RegExs.Match(1);       // Получаем значение первой группировки 
      sName = RegExs.Match(0);       // Получаем совпадение всего шаблона 
      sName = HmsHtmlToText(sName); // Преобразуем html в простой текст 
      
      CreatePodcast(Folder, sName, sLink); // Создаём подкаст с полученным именем и ссылкой
      
    } while (RegExs.SearchAgain);      // Повторяем цикл, если найдено следующее совпадение
    
  } finally { RegExs.Free(); }         // Освобождаем объект из памяти
  
  
  CreatePodcast(FolderItem, '6. Фильмы'               , '/filmy/'); // Создаём подкаст
  if(sPro=='1'){
  CreatePodcast(FolderItem, '6а. 4k'                  , '/filmy/q4/');
  CreatePodcast(FolderItem, '6b. 2k'                  , '/filmy/q2/');
  CreatePodcast(FolderItem, '6c. 1080'                , '/filmy/qh/');  
}
  Folder = FolderItem.AddFolder('7. По категориям фильмы', true);    // Создаём папку
  // Вырезаем нужный блок в переменную sData
  HmsRegExMatch('menu-title">Фильмы<.*?</ul>(.*?)class="lucky"', sHtml, sData);

  // Создаём объект для поиска текста и ищем в цикле по регулярному выражению
  RegEx = TRegExpr.Create('<a[^>]+href="(.*?)".*?</a>'); 
  try {
    if (RegEx.Search(sData)) do {     // Если нашли совпадение, запускаем цикл 
        sLink = RegEx.Match(1);       // Получаем значение первой группировки 
        sName = RegEx.Match(0);       // Получаем совпадение всего шаблона 
        sName = HmsHtmlToText(sName); // Преобразуем html в простой текст 

        CreatePodcast(Folder, sName, sLink); // Создаём подкаст с полученным именем и ссылкой
                                      
    } while (RegEx.SearchAgain);      // Повторяем цикл, если найдено следующее совпадение
  
  } finally { RegEx.Free(); }         // Освобождаем объект из памяти

} 

///////////////////////////////////////////////////////////////////////////////
//                  Г Л А В Н А Я    П Р О Ц Е Д У Р А                       //
{
  HmsRegExMatch('^(.*?//[^/]+)', Podcast[mpiFilePath], gsUrlBase); // Получаем значение в gsUrlBase
  DeleteFolders();   // Удаляем созданное ранее содержимое
  CreateStructure(); // Создаём подкасты
}
