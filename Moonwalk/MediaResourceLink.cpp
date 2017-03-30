﻿// VERSION = 2017.03.29
////////////////////////  Создание  списка  видео   ///////////////////////////
#define mpiJsonInfo 40032
#define mpiKPID     40033
#define DEBUG 0 // Флаг отладки. 1 - при неудачном получении ссылки сохранять файл на рабочий стол ex-fs.log. 0 - не сохранять

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase    = "http://moonwalk.co"; // База для относительных ссылок
int       gnTotalItems = 0;                    // Счётчик созданных элементов
TDateTime gStart       = Now;                  // Время начала запуска скрипта
string    gsTime       = "02:30:00.000";       // Продолжительность видео
string    gsTVDBInfo   = "";
bool gbUseSerialKPInfo = false;

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Установка переменной Podcast: поиск родительской папки, содержащий скрипт
THmsScriptMediaItem GetRoot() {
  Podcast = PodcastItem;
  while ((Trim(Podcast[550])=='') && (Podcast.ItemParent!=nil)) Podcast=Podcast.ItemParent;
  return Podcast;
}

///////////////////////////////////////////////////////////////////////////////
// Получение информации с Kinopoisk о сериале
void LoadKPSerialInfo() {
  string sID, sData, sHtml, sName, sVal, sHeaders, sYear; int nEpisode, nSeason, n; TRegExpr RE;
  if (gsTVDBInfo!='') return;
  sID = Trim(PodcastItem[100500]); // Получаем запомненный kinopoisk ID
  if (sID=='') HmsRegExMatch('/images/(film|film_big)/(\\d+)', mpThumbnail, sID, 2); // Или пытаемся его получить из картинки
  if ((sID!='') && (sID!='0')) {
    // Проверяем, была ли уже загружена информация для такого количества серий
    if ((PodcastItem[100508]!='') && (PodcastItem[100508]==PodcastItem[100509])) {
      gsTVDBInfo = PodcastItem[100507];
      return;
    }
    sHeaders = 'Referer: https://kinopoisk.ru/\r\n'+
               'Accept-Encoding: gzip, deflate\r\n'+
               'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0\r\n';
    sHtml = HmsDownloadURL("https://www.kinopoisk.ru/film/"+sID+"/episodes/", sHeaders, true);
    if (HmsRegExMatch2('<td[^>]+class="news">([^<]+),\\s+(\\d{4})', sHtml, sName, sYear)) {
      // Если получили оригинальное название сериала - пробуем загрузить инфу с картинками
      sName = ReplaceStr(sName, ' ', '_');
      gsTVDBInfo = HmsUtf8Decode(HmsDownloadURL('http://wonky.lostcut.net/tvdb.php?n='+sName+'&y='+sYear, sHeaders, true));
    }
    if (gsTVDBInfo=='') {
      RE = TRegExpr.Create('(<h[^>]+moviename-big.*?)</h', PCRE_SINGLELINE);
      nSeason  = 1;
      nEpisode = 1;
      if (RE.Search(sHtml)) do {
        sName = HmsHtmlToText(re.Match());
        if (HmsRegExMatch("Сезон\\s*?(\\d+)", sName, sVal)) { nSeason=StrToInt(sVal); nEpisode=1; continue; }
        gsTVDBInfo += 's'+Str(nSeason)+'e'+Str(nEpisode)+'=;t='+sName+'|';
        nEpisode++;
      } while (RE.SearchAgain());
    }
    if (gsTVDBInfo=='') gsTVDBInfo = '-';
  }
  PodcastItem[100507] = gsTVDBInfo;          // Запоминаем инфу 
  PodcastItem[100509] = PodcastItem[100508]; // для такого количества серий
}

///////////////////////////////////////////////////////////////////////////////
// Получение реального названия и картинки для серии сериала из gsTVDBInfo
void GetSerialInfo(THmsScriptMediaItem Item, int nSeason, int nEpisode) {
  string sName, sImg;
  if (!gbUseSerialKPInfo ) return; if (gsTVDBInfo=='') LoadKPSerialInfo();
  if (HmsRegExMatch2('s'+Str(nSeason)+'e'+Str(nEpisode)+'=(.*?);t=(.*?)\\|', gsTVDBInfo, sImg, sName)) {
    if (sImg !="") Item[mpiThumbnail] = sImg;
    if (sName!="") Item[mpiTitle    ] = Format("%.2d %s", [nEpisode, sName]);
    Item[mpiSeriesEpisodeNo   ] = nEpisode;
    Item[mpiSeriesSeasonNo    ] = nSeason;
    Item[mpiSeriesEpisodeTitle] = sName;
    Item[mpiSeriesTitle       ] = PodcastItem[mpiSeriesTitle];
  }
}

///////////////////////////////////////////////////////////////////////////////
// Формирование ссылки для воспроизведения через HDSDump.exe
void HDSLink(string sLink, string sQual = '') {
  string sData, sExePath, sVal;
  sExePath = ProgramPath+'\\Transcoders\\hdsdump.exe';
  MediaResourceLink = Format('cmd://"%s" --manifest "%s" --outfile "<OUTPUT FILE>" --threads 2', [sExePath, sLink]);
  if (sQual       != '') MediaResourceLink += ' --quality ' + sQual;
  if (mpTimeStart != '') MediaResourceLink += ' --skip '    + mpTimeStart;
  PodcastItem[mpiTimeSeekDisabled] = true;
  // Получение длительности видео, если она не установлена
  if ((Trim(PodcastItem[mpiTimeLength])=='') || (RightCopy(PodcastItem[mpiTimeLength], 6)=='00.000')) {
    sData = HmsDownloadUrl(sLink, 'Referer: '+sLink, true);
    if (HmsRegExMatch('<duration>(\\d+)', sData, sVal))
      PodcastItem.Properties[mpiTimeLength] = StrToInt(sVal);
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Получение ссылки с moonwalk.cc
void GetLink_Moonwalk(string sLink) {
  string sHtml, sData, sPage, sPost, sManifest, sVer, sSec, sQual, sVal, sServ, sRet, sCookie, sVar;
  int i; float f; TRegExpr RE; bool bHdsDump, bQualLog;
  
  string sHeaders = sLink+'\r\n'+
                    'Accept-Encoding: identity\r\n'+
                    'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0\r\n';
  
  // Проверка установленных дополнительных параметров
  HmsRegExMatch('--quality=(\\w+)', mpPodcastParameters, sQual);
  bHdsDump = Pos('--hdsdump'      , mpPodcastParameters) > 0;
  bQualLog = Pos('--qualitylog'   , mpPodcastParameters) > 0;
  
  sLink = ReplaceStr(sLink, 'moonwalk.co', 'moonwalk.cc');
  sLink = ReplaceStr(sLink, 'moonwalk.pw', 'moonwalk.cc');
  HmsRegExMatch2('//(.*?)(/.*)', sLink, sServ, sPage);
  sHtml = HmsSendRequestEx(sServ, sPage, 'GET', 'application/x-www-form-urlencoded; Charset=UTF-8', sHeaders, '', 80, 0, sRet, true);
  if (HmsRegExMatch('<iframe[^>]+src="(http.*?)"', sHtml, sLink)) {
    sLink = ReplaceStr(sLink, 'moonwalk.co', 'moonwalk.cc');
    sLink = ReplaceStr(sLink, 'moonwalk.pw', 'moonwalk.cc');
    HmsRegExMatch2('//(.*?)(/.*)', sLink, sServ, sPage);
    sHtml = HmsSendRequestEx(sServ, sPage, 'GET', 'application/x-www-form-urlencoded; Charset=UTF-8', sHeaders, '', 80, 0, sRet, true);
  }
  if (HmsRegExMatch('"csrf-token"\\s+content="(.*?)"', sHtml, sVal)) sHeaders += 'X-CSRF-Token:'+sVal+'\r\n';
  sCookie = "";
  if (HmsRegExMatch('Set-Cookie:', sRet, '')) {
    while(HmsRegExMatch2('Set-Cookie:\\s*([^;]+)(.*)', sRet, sVal, sRet, PCRE_SINGLELINE)) sCookie += " "+sVal+";";
  }
  if (!HmsRegExMatch('(https?://.*?)/', sLink, sVal)) sVal = 'http://moonwalk.cc';
  sHeaders += 'Cookie:'+sCookie+'\r\n'+
              'Origin: '+sVal+'\r\n'+
              'X-Requested-With: XMLHttpRequest\r\n';
  // Поиск дополнительных HTTP заголовков, которые нужно установить
  if (HmsRegExMatch('ajaxSetup\\([^)]+headers:(.*?)}', sHtml, sPage)) {
    while(HmsRegExMatch3('["\']([\\w-_]+)["\']\\s*?:\\s*?["\'](\\w+)["\'](.*)', sPage, sVal, sVer, sPage, PCRE_SINGLELINE)) sHeaders += sVal+': '+sVer+'\r\n';
  }
  
  if (!HmsRegExMatch2('/new_session.*?(\\w+)\\s*?=\\s*?\\{(.*?)\\}', sHtml, sVal, sPost)) {
    HmsLogMessage(2, mpTitle+': Не найдены параметры new_session на странице.');
  }
  sPost = ReplaceStr(sPost, ':', '=');
  sPost = ReplaceStr(sPost, ' ', '' );
  sPost = ReplaceStr(sPost, "'", '' );
  sPost = ReplaceStr(sPost, ',', '&');
  sPost = ReplaceStr(sPost, 'condition_detected?1=', '');
  sPost = HmsRemoveLineBreaks(sPost);
  // Замена имён переменных их значениями в параметрах запроса
  sData = sPost;
  while(HmsRegExMatch2('.(=(\\w+).*)', sData, sData, sVar)) {
    if (HmsRegExMatch('var\\s'+sVar+'\\s*=\\s*[\'"](.*?)[\'"]', sHtml, sVal))
      sPost = ReplaceStr(sPost, '='+sVar, '='+sVal);
  }
  // Замена имён переменных их значениями в параметрах запроса, установленных после
  sData = sHtml;
  while(HmsRegExMatch3('.(post_method\\.(\\w+)\\s*=\\s*(\\w+).*)', sData, sData, sVar, sVal)) {
    if (HmsRegExMatch('var\\s'+sVal+'\\s*=\\s*[\'"](.*?)[\'"]', sHtml, sVal))
      sPost += '&'+sVar+'='+sVal;
  }
  HmsRegExMatch('//(.*?)/', sLink, sServ);
  sData = HmsSendRequest(sServ, '/sessions/new_session', 'POST', 'application/x-www-form-urlencoded; Charset=UTF-8', sHeaders, sPost, 80, true);
  sData = HmsJsonDecode(sData);
  
  if (bHdsDump && HmsRegExMatch('"manifest_f4m"\\s*?:\\s*?"(.*?)"', sData, sLink)) {
    
    HDSLink(sLink, sQual);
    
  } else if (HmsRegExMatch('"manifest_m3u8"\\s*?:\\s*?"(.*?)"', sData, sLink)) {
    MediaResourceLink = ' ' + sLink;
    // Получение длительности видео, если она не установлена
    // ------------------------------------------------------------------------
    sData = HmsDownloadUrl(sLink, 'Referer: '+sHeaders, true);
    sVal  = Trim(PodcastItem[mpiTimeLength]);
    if ((sVal=='') || (RightCopy(sVal, 6)=='00.000')) {
      if (HmsRegExMatch('(http.*?)[\r\n$]', sData, sLink)) {
        sHtml = HmsDownloadUrl(sLink, 'Referer: '+sHeaders, true);
        RE = TRegExpr.Create('#EXTINF:(\\d+.\\d+)', PCRE_SINGLELINE);
        if (RE.Search(sHtml)) do f += StrToFloatDef(RE.Match(1), 0); while (RE.SearchAgain());
        RE.Free;
        PodcastItem.Properties[mpiTimeLength] = Round(f);
      }
    }
    // ------------------------------------------------------------------------
    
    // Если установлен ключ --quality или в настройках подкаста выставлен приоритет выбора качества
    // ------------------------------------------------------------------------
    string sSelectedQual = '', sMsg, sHeight; int iMinPriority = 99, iPriority; 
    if ((sQual!='') || (mpPodcastMediaFormats!='')) {
      TStringList QLIST = TStringList.Create();
      // Собираем список ссылок разного качества
      RE = TRegExpr.Create('#EXT-X-STREAM-INF:RESOLUTION=\\d+x(\\d+).*?[\r\n](http.+)$', PCRE_MULTILINE);
      if (RE.Search(sData)) do {
        sHeight = Format('%.5d', [StrToInt(RE.Match(1))]);
        sLink   = RE.Match(2);
        QLIST.Values[sHeight] = sLink;
        iPriority = HmsMediaFormatPriority(StrToInt(sHeight), mpPodcastMediaFormats);
        if ((iPriority >= 0) && (iPriority < iMinPriority)) {
          iMinPriority  = iPriority;
          sSelectedQual = sHeight;
        }
      } while (RE.SearchAgain());
      RE.Free;
      QLIST.Sort();
      if (QLIST.Count > 0) {
        if      (sQual=='low'   ) sSelectedQual = QLIST.Names[0];
        else if (sQual=='medium') sSelectedQual = QLIST.Names[Round((QLIST.Count-1) / 2)];
        else if (sQual=='high'  ) sSelectedQual = QLIST.Names[QLIST.Count - 1];
      }
      if (sSelectedQual != '') MediaResourceLink = ' ' + QLIST.Values[sSelectedQual];
      if (bQualLog) {
        sMsg = 'Доступное качество: ';
        for (i = 0; i < QLIST.Count; i++) {
          if (i>0) sMsg += ', ';
          sMsg += IntToStr(StrToInt(QLIST.Names[i])); // Обрезаем лидирующие нули
        }
        if (sSelectedQual != '') sSelectedQual = IntToStr(StrToInt(sSelectedQual));
        else sSelectedQual = 'Auto';
        sMsg += '. Выбрано: ' + sSelectedQual;
        HmsLogMessage(1, mpTitle+'. '+sMsg);
      }
      QLIST.Free;
    }
    // ------------------------------------------------------------------------
    
  } else if (HmsRegExMatch('"manifest_mp4"\\s*?:\\s*?"(.*?)"', sData, sLink)) {
    sData = HmsDownloadURL(sLink, 'Referer: '+sHeaders);
    // Поддержка установленных приоритетов качества в настройках подкаста
    TJsonObject JSON = TJsonObject.Create();
    int height, selHeight=0, minPriority=99, priority, maxHeight;
    if (sQual=='medium') sQual='480'; if (sQual=='low') sQual='360';
    maxHeight = StrToIntDef(sQual, 4320);
    try {
      JSON.LoadFromString(sData);
      for (i=0; i<JSON.Count; i++) {
        sVer  = JSON.Names[i];
        sLink = JSON.S[sVer];
        height = StrToIntDef(sVer, 0);
        if ((sQual!='') && (sQual==sVer)) { MediaResourceLink = sLink; selHeight = height; break; }
        if (mpPodcastMediaFormats!='') {
          priority = HmsMediaFormatPriority(height, mpPodcastMediaFormats);
          if ((priority>=0) && (priority<minPriority)) {
            MediaResourceLink = sLink; minPriority = priority;
          }
        } else if ((height > selHeight) && (height <= maxHeight)) {
          MediaResourceLink = sLink; selHeight = height;
        }
      }
    } finally { JSON.Free(); }
    
  } else {
    HmsLogMessage(2, mpTitle+': Ошибка получения данных от new_session.');
    if (DEBUG==1) {
      if (ServiceMode) sVal = SpecialFolderPath(0x37); // Общая папака "Видео"
        else           sVal = SpecialFolderPath(0);    // Рабочий стол
        sVal = IncludeTrailingBackslash(sVal)+'Moonwalk.log';
      HmsStringToFile(sHeaders+'\r\nsHtml:\r\n'+sHtml+'\r\nsPost:\r\n'+sPost+'\r\nsData:\r\n'+sData, sVal);
      HmsLogMessage(1, mpTitle+': Создан лог файл '+sVal);
    }
  }
} // Конец функции поулчения ссылки с moonwalk.cc

///////////////////////////////////////////////////////////////////////////////
// Создание информационной ссылки
void CreateInfoItem(string sName, TStrings INFO) {
  THmsScriptMediaItem Item; string sVal = Trim(INFO.Values[sName]);
  if ((sVal=="") || (sVal=="-")) return;
  Item = HmsCreateMediaItem('Info'+IntToStr(PodcastItem.ChildCount), PodcastItem.ItemID);
  Item[mpiTitle     ] = sName+': '+sVal;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/vids/info.jpg';
  Item[mpiTimeLength] = 20;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  gnTotalItems++;
  // ---- Формирование и сохранение текста для отображения видео информации ---
  string sInfo='';
  for (int i=0; i<INFO.Count; i++) {
    sName = INFO.Names[i];
    if (HmsRegExMatch('(Описание|Жанр|Название|Постер|Трейлер)', sName, '')) continue;
    sInfo += "<c:#FFC3BD>"+sName+": </c>"+INFO.Values[sName]+" ";
    if (!HmsRegExMatch('(Год|IMDb)', sName, '')) sInfo += "|";
  }
  sInfo = Copy(sInfo, 1, Length(sInfo)-1);
  
  TStrings INFOTEXT = TStringList.Create();  
  INFOTEXT.Values['Poster'] = mpThumbnail;
  INFOTEXT.Values['Title' ] = INFO.Values['Название'];
  INFOTEXT.Values['Info'  ] = Trim(sInfo);
  INFOTEXT.Values['Categ' ] = INFO.Values['Жанр'    ];
  INFOTEXT.Values['Descr' ] = INFO.Values['Описание'];
  Item[1001001] = INFOTEXT.Text;
  INFOTEXT.Free();
  // ---------------------------------
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void CreateErrorItem(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err'+IntToStr(PodcastItem.ChildCount), PodcastItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки или подкаста
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem ParentFolder, string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = ParentFolder.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle     ] = sName; // Присваиваем наименование
  Item[mpiThumbnail ] = sImg;  // Картинка
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, -gnTotalItems, 0, 0)); // Для обратной сортировки по дате создания
  Item[100500       ] = PodcastItem[100500];
  gnTotalItems++;             // Увеличиваем счетчик созданных элементов
  return Item;                // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg, string sTime='') {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  Item[mpiTimeLength] = sTime;
  gnTotalItems++;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// ----------------- Формирование видео с картинкой с информацией о фильме ----
bool VideoPreview() {
  string sVal, sFileImage, sPoster, sTitle, sDescr, sCateg, sInfo, sLink, sData;
  int xMargin=7, yMargin=10, nSeconds=10, n; 
  string gsCacheDir      = IncludeTrailingBackslash(HmsTempDirectory)+'Moonwalk/';
  string gsPreviewPrefix = "Moonwalk";
  float nH=cfgTranscodingScreenHeight, nW=cfgTranscodingScreenWidth;
  // Проверяем и, если указаны в параметрах подкаста, выставляем значения смещения от краёв
  if (HmsRegExMatch('--xmargin=(\\d+)', mpPodcastParameters, sVal)) xMargin=StrToInt(sVal);
  if (HmsRegExMatch('--ymargin=(\\d+)', mpPodcastParameters, sVal)) yMargin=StrToInt(sVal);
  
  if (Trim(PodcastItem[1001001])=='') return; // Если нет инфы - выходим быстро!
    TStrings INFO = TStringList.Create();       // Создаём объект TStrings
  INFO.Text  = PodcastItem[1001001];          // И загружаем туда информацию
  sPoster = INFO.Values['Poster'];            // Постер
  sTitle  = INFO.Values['Title' ];            // Самая верхняя надпись - Название
  sCateg  = INFO.Values['Genre' ];            // Жанр
  sInfo   = INFO.Values['Info'  ];            // Блок информации
  sDescr  = INFO.Values['Descr' ];
  if (sTitle=='') sTitle = ' ';
  ForceDirectories(gsCacheDir);
  sFileImage = ExtractShortPathName(gsCacheDir)+'videopreview_'; // Файл-заготовка для сохранения картинки
  sDescr = Copy(sDescr, 1, 3000); // Если блок описания получился слишком большой - обрезаем
  
  INFO.Text = ""; // Очищаем объект TStrings для формирования параметров запроса
  INFO.Values['prfx'  ] = gsPreviewPrefix;  // Префикс кеша сформированных картинок на сервере
  INFO.Values['title' ] = sTitle;           // Блок - Название
  INFO.Values['info'  ] = sInfo;            // Блок - Информация
  INFO.Values['categ' ] = sCateg;           // Блок - Жанр/категории
  INFO.Values['descr' ] = sDescr;           // Блок - Описание фильма
  INFO.Values['mlinfo'] = '20';             // Максимальное число срок блока Info
  INFO.Values['w' ] = IntToStr(Round(nW));  // Ширина кадра
  INFO.Values['h' ] = IntToStr(Round(nH));  // Высота кадра
  INFO.Values['xm'] = IntToStr(xMargin);    // Отступ от краёв слева/справа
  INFO.Values['ym'] = IntToStr(yMargin);    // Отступ от краёв сверху/снизу
  INFO.Values['bg'] = 'http://www.pageresource.com/wallpapers/wallpaper/noir-blue-dark_3512158.jpg'; // Катринка фона (кэшируется на сервере) 
  INFO.Values['fx'] = '3'; // Номер эффекта для фона: 0-нет, 1-Blur, 2-more Blur, 3-motion Blur, 4-radial Blur
  INFO.Values['fztitle'] = IntToStr(Round(nH/14)); // Размер шрифта блока названия (тут относительно высоты кадра)
  INFO.Values['fzinfo' ] = IntToStr(Round(nH/22)); // Размер шрифта блока информации
  INFO.Values['fzcateg'] = IntToStr(Round(nH/26)); // Размер шрифта блока жанра/категории
  INFO.Values['fzdescr'] = IntToStr(Round(nH/18)); // Размер шрифта блока описания
  // Если текста описания больше чем нужно - немного уменьшаем шрифт блока
  if (Length(sDescr)>890) INFO.Values['fzdescr'] = IntToStr(Round(nH/20));
  // Если есть постер, задаём его параметры отображения (где, каким размером)
  if (sPoster!='') {
    INFO.Values['wpic'  ] = IntToStr(Round(nW/4)); // Ширина постера (1/4 ширины кадра)
    INFO.Values['xpic'  ] = '10';    // x-координата постера
    INFO.Values['ypic'  ] = '10';    // y-координата постера
    if (mpFilePath=='InfoUpdate') {
      INFO.Values['wpic'  ] = IntToStr(Round(nW/6)); // Ширина постера (1/6 ширины кадра)
      INFO.Values['xpic'  ] = IntToStr(Round(nW/2 - nW/12)); // центрируем
    }
    INFO.Values['urlpic'] = sPoster; // Адрес изображения постера
  }
  sData = '';  // Из установленных параметров формируем строку POST запроса
  for (n=0; n<INFO.Count; n++) sData += '&'+Trim(INFO.Names[n])+'='+HmsHttpEncode(INFO.Values[INFO.Names[n]]);
  INFO.Free(); // Освобождаем объект из памяти, теперь он нам не нужен
  // Делаем POST запрос не сервер формирования картинки с информацией
  sLink = HmsSendRequestEx('wonky.lostcut.net', '/videopreview.php?p='+gsPreviewPrefix, 'POST', 
  'application/x-www-form-urlencoded', '', sData, 80, 0, '', true);
  // В ответе должна быть ссылка на сформированную картинку
  if (LeftCopy(sLink, 4)!='http') {HmsLogMessage(2, 'Ошибка получения файла информации videopreview.'); return;}
  // Сохраняем сформированную картинку с информацией в файл на диске
  HmsDownloadURLToFile(sLink, sFileImage);
  // Копируем и нумеруем файл картики столько раз, сколько секунд мы будем её показывать
  for (n=1; n<=nSeconds; n++) CopyFile(sFileImage, sFileImage+Format('%.3d.jpg', [n]), false);
  // Для некоторых телевизоров (Samsung) видео без звука отказывается проигрывать, поэтому скачиваем звук тишины
  char sFileMP3 = ExtractShortPathName(HmsTempDirectory)+'\\silent.mp3';
  try {
    if (!FileExists(sFileMP3)) HmsDownloadURLToFile('http://wonky.lostcut.net/mp3/silent.mp3', sFileMP3);
    sFileMP3 = '-i "'+sFileMP3+'"';
  } except { sFileMP3=''; }
  // Формируем из файлов пронумерованных картинок и звукового команду для формирования видео
  MediaResourceLink = Format('%s -f image2 -r 1 -i "%s" -c:v libx264 -pix_fmt yuv420p ', [sFileMP3, sFileImage+'%03d.jpg']);
}

///////////////////////////////////////////////////////////////////////////////
// Получение названий и картинок серий сериала
void GetSerialInfoFromImdb(string sName, TStrings INFO) {
  string sData, sLink, sID, sKey, sImg, sVal; int nSeason, nEpisode, nYear;
  TJsonObject JSON, MOVIE, SEASON, EPISODE; TJsonArray SEASONS, EPISODES;
  
  sLink = 'http://www.myapifilms.com/imdb/idIMDB?format=json&language=ru&token=c5c8d3f3-0524-4c66-90df-e2e727b2e628&&seasons=1&title='+HmsHttpEncode(sName);
  sData = HmsRemoveLineBreaks(HmsDownloadURL(sLink, '', true));
  
  JSON = TJsonObject.Create();
  try {
    JSON.LoadFromString(sData);
    nYear   = StrToIntDef(LeftCopy(JSON.O['0'].S['year'], 4), 0);
    MOVIE   = JSON.O['data\\movies\\0'];
    if (MOVIE == nil) return;
    SEASONS = MOVIE['seasons'].AsArray();
    if (SEASONS == nil) return;
    for (nSeason=0; nSeason<SEASONS.Length; nSeason++) {
      EPISODES = SEASONS[nSeason].A['episodes'];
      for (nEpisode=0; nEpisode<EPISODES.Length; nEpisode++) {
        EPISODE = EPISODES[nEpisode];
        sImg = EPISODE.S['urlPoster']; if (sImg=='') continue;
        sKey = IntToStr(nSeason+1)+'x'+EPISODE.S['episode'];
        INFO.Values[sKey] = sImg;
      }
    } 
    
  } finally { JSON.Free; } 
}

///////////////////////////////////////////////////////////////////////////////
// Создание списка серий сериала с Moonwalk.cc
void CreateMoonwallkLinks(string sLink) {
  String sHtml, sData, sSerie, sVal, sHeaders, sEpisodeTitle, sShowTitle, sID;
  int n, nEpisode, nSeason; TRegExpr RE; THmsScriptMediaItem Item, Folder = PodcastItem;
  
  sHeaders = sLink+'/\r\n'+
             'Accept-Encoding: gzip, deflate\r\n'+
             'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0\r\n'+
             'X-Requested-With: XMLHttpRequest\r\n';
  
  gbUseSerialKPInfo = Pos("--usekpinfo", mpPodcastParameters) > 0;
  
  sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders, true);
  if (HmsRegExMatch('<body>\\s*?</body>', sHtml, '')) {
    sLink = ReplaceStr(sLink, 'moonwalk.pw', 'moonwalk.cc');
    sLink = ReplaceStr(sLink, 'moonwalk.co', 'moonwalk.cc');
    sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders, true);
  }
  if (HmsRegExMatch('<iframe[^>]+src="(http.*?)"', sHtml, sLink)) sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders, true);
  sHtml = HmsRemoveLineBreaks(HmsUtf8Decode(sHtml));
  
  if (Trim(mpSeriesTitle)=='') { PodcastItem[mpiSeriesTitle] = mpTitle; HmsRegExMatch('^(.*?)[\\(\\[]', mpTitle, PodcastItem[mpiSeriesTitle]); }

  // Подсчитываем количество серий и запоминаем в укромном месте
  RE = TRegExpr.Create('(<option[^>]+value="(.*?)".*?</option>)', PCRE_SINGLELINE);
  n = 0; if (RE.Search(sHtml)) do n++; while (RE.SearchAgain()); PodcastItem[100508] = Str(n);

  if (HmsRegExMatch('<select[^>]+id="episode"(.*?)</select>', sHtml, '')) {
    if (HmsRegExMatch('season=(\\d+)', sLink, sVal)) {
      gsTime  = '00:45:00.000';
      nSeason = StrToInt(sVal);
      HmsRegExMatch('<select[^>]+id="episode"(.*?)</select>', sHtml, sData);
      RE = TRegExpr.Create('(<option[^>]+value="(.*?)".*?</option>)', PCRE_SINGLELINE);
      if (RE.Search(sData)) do {
        sSerie = ReplaceStr(HmsHtmlToText(RE.Match(1)), '/', '-');
        // Форматируем номер в два знака
        if (HmsRegExMatch("(\\d+)", sSerie, sVal)) {
          nEpisode = StrToInt(sVal);
          sSerie   = Format("%.2d %s", [nEpisode, ReplaceStr(sSerie, sVal, "")]);
        }
        Item = CreateMediaItem(Folder, sSerie, sLink+'&episode='+RE.Match(2), mpThumbnail, gsTime);
        GetSerialInfo(Item, nSeason, nEpisode);
      } while (RE.SearchAgain());
      RE.Free();
    } else if (HmsRegExMatch('<select[^>]+id="season"(.*?)</select>', sHtml, sData)) {
      RE = TRegExpr.Create('(<option[^>]+value="(.*?)".*?</option>)', PCRE_SINGLELINE);
      // Подсчитываем количество сезонов
      n = 0; if (RE.Search(sData)) do { n++; HmsRegExMatch("(\\d+)", HmsHtmlToText(RE.Match(1)), sVal); } while (RE.SearchAgain());
      if ( (n == 1) && (sVal == '1') ) {
        // Если сезон всего один и номер его = 1, то сразу выкатываем серии
        gsTime   = '00:45:00.000';
        nSeason  = 1;
        HmsRegExMatch('<select[^>]+id="episode"(.*?)</select>', sHtml, sData);
        if (RE.Search(sData)) do {
          sSerie = ReplaceStr(HmsHtmlToText(RE.Match(1)), '/', '-');
          // Форматируем номер в два знака
          if (HmsRegExMatch("(\\d+)", sSerie, sVal)) {
            nEpisode = StrToInt(sVal);
            sSerie   = Format("%.2d %s", [nEpisode, ReplaceStr(sSerie, sVal, "")]);
          }
          Item = CreateMediaItem(Folder, sSerie, sLink+'?season=1&episode='+RE.Match(2), mpThumbnail, gsTime);
          GetSerialInfo(Item, nSeason, nEpisode);
        } while (RE.SearchAgain());
      } else {
        if (RE.Search(sData)) do {
          sSerie = ReplaceStr(HmsHtmlToText(RE.Match(1)), '/', '-');
          Item = CreateFolder(Folder, sSerie, sLink+'?season='+RE.Match(2), mpThumbnail);
          Item[mpiSeriesTitle] = PodcastItem[mpiSeriesTitle];
        } while (RE.SearchAgain());
      }
      RE.Free();
    }
  } else {
    CreateMediaItem(Folder, mpTitle, sLink, mpThumbnail, gsTime);
  }
}

///////////////////////////////////////////////////////////////////////////////
void SetInfo(string sHtml, string sPattern, string sName, TStrings INFO) {
  string sVal;
  if (HmsRegExMatch(sPattern, sHtml, sVal)) {
    sVal = ReplaceStr(HmsHtmlToText(sVal), '|', '');
    if ((sVal!='') && (sVal!='-'))
      INFO.Values[sName] = sVal;
  } 
}

///////////////////////////////////////////////////////////////////////////////
// Создание информационных ссылок (Жанр, Страна, Рейтинг и проч.)
void CreateInfoItems() {
  string sHtml, sVal, sVal2, sKPID; TStrings INFO;
  if (!HmsRegExMatch('kinopoisk.ru/images/film/(\\d+)', mpThumbnail, sKPID)) return;
  sHtml = HmsDownloadURL('http://www.kinopoisk.ru/film/'+sKPID+'/', 'Referer: http://ivi.ru', true);
  sHtml = HmsRemoveLineBreaks(HmsUtf8Decode(sHtml));
  
  INFO = TStringList.Create;
  SetInfo(sHtml, '(<h1.*?</h1>)'           , 'Название', INFO);
  SetInfo(sHtml, '>год</td>(.*?)</tr>'     , 'Год'     , INFO);
  SetInfo(sHtml, '>страна</td>(.*?)</tr>'  , 'Страна'  , INFO);
  SetInfo(sHtml, '>слоган</td>(.*?)</tr>'  , 'Слоган'  , INFO);
  SetInfo(sHtml, '>режиссер</td>(.*?)</tr>', 'Режиссер', INFO);
  SetInfo(sHtml, '>сценарий</td>(.*?)</tr>', 'Сценарий', INFO);
  SetInfo(sHtml, '>продюсер</td>(.*?)</tr>', 'Продюсер', INFO);
  SetInfo(sHtml, '>оператор</td>(.*?)</tr>', 'Оператор', INFO);
  SetInfo(sHtml, '>бюджет</td>(.*?)</tr>'  , 'Бюджет'  , INFO);
  SetInfo(sHtml, '>возраст</td>(.*?)</tr>' , 'Возраст' , INFO);
  SetInfo(sHtml, '>жанр</td>(.*?)</(span|tr)>', 'Жанр' , INFO);
  SetInfo(sHtml, '>IMDb:(.*?)</'           , 'IMDb'    , INFO);
  
  SetInfo(sHtml, '>сборы в США</td>(.*?)</(a|tr)>'   , 'Сборы в США'   , INFO);
  SetInfo(sHtml, '>сборы в мире</td>(.*?)</(a|tr)>'  , 'Сборы в мире'  , INFO);
  SetInfo(sHtml, '>сборы в России</td>(.*?)</(a|tr)>', 'Сборы в России', INFO);
  SetInfo(sHtml, '>премьера (мир)</td>(.*?)</(a|tr)>', 'Премьера (мир)', INFO);
  SetInfo(sHtml, '>премьера (РФ)</td>(.*?)</(a|tr)>' , 'Премьера (РФ)' , INFO);
  
  if (HmsRegExMatch('"rating_ball">(.*?)</(span|tr)>', sHtml, sVal)) {
    if (HmsRegExMatch('"ratingCount">(.*?)</(span|tr)>', sHtml, sVal2)) sVal += ' ('+HmsHtmlToText(sVal2)+')';
    INFO.Values['Рейтинг КП'] = HmsHtmlToText(sVal);
  }
  if (HmsRegExMatch('>время</td>(.*?)</tr>', sHtml, sVal)) {
    if (HmsRegExMatch('(\\d+)\\s*?мин.', sVal, sVal)) {
      INFO.Values['Продолжительность'] = HmsTimeFormat(StrToInt(sVal) * 60);
    }
  }
  SetInfo(sHtml, '(<div[^>]+"description".*?</div>)', 'Описание', INFO);
  
  if (HmsRegExMatch('(/film/\\d+/video/)\\d+/', sHtml, sVal)) {
    sVal = 'http://www.kinopoisk.ru'+Trim(sVal);
    if (Pos('--trailersfolder', mpPodcastParameters) > 0)
      CreateFolder(PodcastItem, 'Трейлеры', sVal);
    else
      CreateTrailersLinks(sVal);
  }
  
  // Создаём выборочно информационные ссылки
  CreateInfoItem('Страна'    , INFO);
  CreateInfoItem('Жанр'      , INFO);
  CreateInfoItem('Режиссер'  , INFO);
  CreateInfoItem('IMDb'      , INFO);
  CreateInfoItem('Рейтинг КП', INFO);
  INFO.Free;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на трейлеры
void CreateTrailersLinks(string sLink) {
  string sHtml, sName, sTitle, sVal, sImg;
  TRegExpr RE1, RE2; THmsScriptMediaItem Item;
  
  sHtml = HmsDownloadURL(sLink, 'Referer: '+sLink, true);
  RE1 = TRegExpr.Create('<!-- ролик -->(.*?)<!-- /ролик -->', PCRE_SINGLELINE);
  RE2 = TRegExpr.Create('<a[^>]+href="/getlink.php[^>]+link=([^>]+\\.(mp4|mov|flv)).*?</a>', PCRE_SINGLELINE);
  if (RE1.Search(sHtml)) do {
    if (!HmsRegExMatch('(<a[^>]+href="/film/\\d+/video/\\d+/".*?</a>)', RE1.Match, sTitle)) continue;
    sTitle = HmsHtmlToText(sTitle);
    if (HmsRegExMatch('["\']previewFile["\']:\\s*?["\'](.*?)["\']', RE1.Match, sVal)) {
      sImg = 'http://kp.cdn.yandex.net/'+sVal;
    }
    if (RE2.Search(RE1.Match)) do {
      sLink = RE2.Match(1);
      sName = sTitle+' '+HmsHtmlToText(RE2.Match(0));
      Item  = PodcastItem.FindItemByProperty(mpiTitle, sName);
      if ((Item != null) && (Item != nil))
        Item[mpiFilePath] = sLink;
      else 
        CreateMediaItem(PodcastItem, sName, sLink, sImg, '00:03:50.000');
    } while (RE2.SearchAgain());
  
  } while (RE1.SearchAgain());
  RE1.Free; RE2.Free;
}

///////////////////////////////////////////////////////////////////////////////
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
///////////////////////////////////////////////////////////////////////////////
{
  
  if (PodcastItem.IsFolder) {
    PodcastItem.DeleteChildItems();
    
    if (HmsRegExMatch('^(.*?kinopoisk.ru/film/\\d+/video/)', mpFilePath, '')) {
      CreateTrailersLinks(mpFilePath);

    } else {
      CreateMoonwallkLinks(mpFilePath);
      if ((Pos('?season', mpFilePath) < 1) && (Pos('--infoitems', mpPodcastParameters) > 0)) {
        CreateInfoItems();
      }
    }
    
  } else {

    if (LeftCopy(mpFilePath, 4)=='Info')
      VideoPreview();
    else if (HmsRegExMatch('\\.(mp4|mov|flv)$', mpFilePath, ''))
      MediaResourceLink = mpFilePath;
    else
      GetLink_Moonwalk(mpFilePath);
    
  }
  
}
