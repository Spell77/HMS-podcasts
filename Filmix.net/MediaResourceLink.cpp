// 2018.12.16  Collaboration: WendyH, Big Dog, михаил, Spell
//////////////////  Получение ссылок на медиа-ресурс   ////////////////////////
#define mpiSeriesInfo 10323  // Идентификатор для хранения информации о сериях
#define mpiJsonInfo   40032
#define mpiKPID       40033
///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase    = ''; // Url база ссылок нашего сайта (берётся из корневого элемента)
bool      gbHttps      = (LeftCopy(gsUrlBase, 5)=='https');
string    gnTime       = '01:40:00.000';
int       gnTotalItems = 0;
int       gnQual       = 0;  // Минимальное качество для отображения
int       mpiCountry   = 10012; // Идентификаторы для хранения дополнительной
int       mpiTranslate = 10013; // информации в свойствах подкаста
int       mpiQuality   = 10014;
int       gnDefaultTime = 6000;
int       mpiVideoMessage = 1001001;
int       gnItemsAdded = 0;
TDateTime gStart       = Now;
string    gsSeriesInfo = ''; // Информация о сериях сериала (названия)
string    gsIDBase     = '';
string    gsPreviewPrefix = "filmix";
string    gsHeaders = mpFilePath+'\r\n'+
                      'Accept: application/json, text/javascript, */*; q=0.01\r\n'+
                      'Accept-Encoding: identity\r\n'+
                      'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36\r\n'+
                      'X-Requested-With: XMLHttpRequest\r\n';

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
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg, int nTime, string sGrp='') {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID, sGrp);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = IncTime(gStart,0,-gnTotalItems,0,0); gnTotalItems++;
  Item[mpiTimeLength] = HmsTimeFormat(nTime)+'.000';
  if (HmsRegExMatch('(?:/embed/|v=)([\\w-_]+)', sLink, sImg))
    Item[mpiThumbnail] = 'http://img.youtube.com/vi/'+sImg+'/1.jpg';
  return Item;
}
///////////////////////////////////////////////////////////////////////////////
// Декодирование ссылок для HTML5 плеера
string Html5Decode(string sEncoded) {
  if ((sEncoded=="") || (Pos(".", sEncoded) > 0)) return sEncoded;
  if (sEncoded[1]=="#") sEncoded = Copy(sEncoded, 2, Length(sEncoded)-1);
  string sDecoded = "";
  for (int i=1; i <= Length(sEncoded); i+=3)
    sDecoded += "\\u0" + Copy(sEncoded, i, 3);
  return HmsJsonDecode(sDecoded);
}

//////////////////////////////////////////////////////////////////////////////
// Авторизация на сайте
bool LoginToFilmix() {
  string sName,sNames, sPass, sLink, sData, sPost, sRet, sDomen;
  HmsRegExMatch('//([^/]+)', gsUrlBase, sDomen);
  int nPort = 443, nFlags = 0x10; // INTERNET_COOKIE_THIRD_PARTY;
  
  if ((Trim(mpPodcastAuthorizationUserName)=='') ||
  (Trim(mpPodcastAuthorizationPassword)=='')) {
    ErrorItem('Не указан логин или пароль');
    //return false;
    return true; // Не включена авторизация - работаем так, без неё.
  }
  
  sName = HmsHttpEncode(HmsUtf8Encode(mpPodcastAuthorizationUserName)); // Логин
  sPass = HmsHttpEncode(HmsUtf8Encode(mpPodcastAuthorizationPassword)); // Пароль
  sPost = 'login_name='+sName+'&login_password='+sPass+'&login_not_save=1&login=submit';
  //sData = HmsSendRequestEx(sDomen, '/engine/ajax/user_auth.php', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, sPost, nPort, nFlags, sRet, true);
  sData = HmsSendRequestEx(sDomen, '/', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, sPost, nPort, nFlags, sRet, true);
  //sData = HmsSendRequestEx(sDomen, '/engine/ajax/user_auth.php', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, sPost, nPort, nFlags, sRet, true);
  HmsRegExMatch('var dle_user_name="(.*?)";', sData, sNames);
  if (sName != sNames) {
    ErrorItem('Не прошла авторизация на сайте '+sDomen+'. Неправильный логин/пароль?');
    return false;  
  }
  
  return true;
}

////
///////////////////////////////////////////////////////////////////////////////
// ---- Создание ссылки на видео ----------------------------------------------
THmsScriptMediaItem AddMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sGrp='') {
  HmsRegExMatch('\\[.*?\\](.*)', sLink, sLink);
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID, sGrp); // Создаём ссылку
  Item[mpiTitle     ] = sTitle;       // Наименование
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, 0, -gnTotalItems, 0));
  //  Item[mpiDirectLink] = True;
  // Тут наследуем от родительской папки полученные при загрузке первой страницы данные
  Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiGenre, mpiYear, mpiComment]);
  
  gnTotalItems++;                    // Увеличиваем счетчик созданных элементов
  return Item;                       // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// --- Создание папки видео ---------------------------------------------------
THmsScriptMediaItem CreateFolder(string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = PodcastItem.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle     ] = sName;  // Присваиваем наименование
  Item[mpiThumbnail ] = sImg;   // Картинку устанавливаем, которая указана у текущей папки
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, 0, -gnTotalItems, 0));
  
  // Тут наследуем от родительской папки полученные при загрузке первой страницы данные
  Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiGenre, mpiYear, mpiComment]);
  
  gnTotalItems++;               // Увеличиваем счетчик созданных элементов
  return Item;                  // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void ErrorItem(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err', PodcastItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// ---- Создание информационной ссылки ----------------------------------------
void AddInfoItem(string sTitle) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('-Info'+IntToStr(PodcastItem.ChildCount), PodcastItem.ItemID);
  Item[mpiTitle     ] = HmsHtmlToText(sTitle);  // Наименование (Отображаемая информация)
  Item[mpiTimeLength] = 1;       // Т.к. это псевдо ссылка, то ставим длительность 1 сек.
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/vids/info.jpg'; // Ставим иконку информации
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, 0, -gnTotalItems, 0));
  gnTotalItems++;
}

///////////////////////////////////////////////////////////////////////////////
// Вывод видео сообщения с информацией о фильме
void ShowVideoInfo() {
  string sInfo; THmsScriptMediaItem Parent = PodcastItem.ItemParent;
  
  sInfo = '';
  if (Trim(Parent[mpiCountry  ])!='') sInfo += 'Страна: '  +Parent[mpiCountry  ]+"|";
  if (Trim(Parent[mpiTranslate])!='') sInfo += 'Перевод: ' +Parent[mpiTranslate]+"|";
  if (Trim(Parent[mpiQuality  ])!='') sInfo += 'Качество: '+Parent[mpiQuality  ]+"|";
  if (Trim(Parent[mpiDirector ])!='') sInfo += 'Режиссер: '+Parent[mpiDirector ]+"|";
  if (Trim(Parent[mpiActor    ])!='') sInfo += 'В ролях: ' +Parent[mpiActor    ]+"|";
  sInfo = Copy(sInfo, 1, Length(sInfo)-1); // Обрезаем последний символ "|"
  TStrings INFO = TStringList.Create();
  INFO.Values['Poster'] = Parent[mpiThumbnail];
  INFO.Values['Title' ] = Parent[mpiTitle];
  INFO.Values['Genre' ] = Parent[mpiGenre];
  INFO.Values['Info'  ] = sInfo;
  INFO.Values['Descr' ] = ReplaceStr(Parent[mpiComment], "\n", "|");
  PodcastItem[mpiVideoMessage] = INFO.Text;
  INFO.Free();
  VideoPreview();
}

///////////////////////////////////////////////////////////////////////////////
// Формирование видео с картинкой с информацией о фильме
bool VideoPreview() {
  string sVal, sFileImage, sPoster, sTitle, sDescr, sCateg, sInfo, sLink, sData;
  int xMargin=7, yMargin=10, nSeconds=10, n; string sCacheDir;
  float nH=cfgTranscodingScreenHeight, nW=cfgTranscodingScreenWidth;
  // Проверяем и, если указаны в параметрах подкаста, выставляем значения смещения от краёв
  if (HmsRegExMatch('--xmargin=(\\d+)', mpPodcastParameters, sVal)) xMargin=StrToInt(sVal);
  if (HmsRegExMatch('--ymargin=(\\d+)', mpPodcastParameters, sVal)) yMargin=StrToInt(sVal);
  sCacheDir = IncludeTrailingBackslash(HmsTempDirectory);
  
  if (Trim(PodcastItem[mpiVideoMessage])=='') return; // Если нет инфы - выходим быстро!
    TStrings INFO = TStringList.Create();       // Создаём объект TStrings
  INFO.Text  = PodcastItem[1001001];          // И загружаем туда информацию
  sPoster = INFO.Values['Poster'];            // Постер
  sTitle  = INFO.Values['Title' ];            // Самая верхняя надпись - Название
  sCateg  = INFO.Values['Categ' ];            // Жанр
  sInfo   = INFO.Values['Info'  ];            // Блок информации
  sDescr  = INFO.Values['Descr' ];            // Описание
  if (sTitle=='') sTitle = ' ';
  ForceDirectories(sCacheDir);
  sFileImage = ExtractShortPathName(sCacheDir)+'videopreview_'; // Файл-заготовка для сохранения картинки
  sDescr = Copy(sDescr, 1, 5000); // Если блок описания получился слишком большой - обрезаем
  
  INFO.Text = ""; // Очищаем объект TStrings для формирования параметров запроса
  INFO.Values['prfx' ] = gsPreviewPrefix;  // Префикс кеша сформированных картинок на сервере
  INFO.Values['title'] = sTitle;           // Блок - Название
  INFO.Values['info' ] = sInfo;            // Блок - Информация
  INFO.Values['categ'] = sCateg;           // Блок - Жанр/категории
  INFO.Values['descr'] = sDescr;           // Блок - Описание фильма
  INFO.Values['mlinfo'] = '20';            // Максимальное число срок блока Info
  INFO.Values['w' ] = IntToStr(Round(nW)); // Ширина кадра
  INFO.Values['h' ] = IntToStr(Round(nH)); // Высота кадра
  INFO.Values['xm'] = IntToStr(xMargin);   // Отступ от краёв слева/справа
  INFO.Values['ym'] = IntToStr(yMargin);   // Отступ от краёв сверху/снизу
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
  MediaResourceLink = MediaResourceLink;
}


// ----------------------------------------------------------------------------
void CreateVideoInfoItem(char sName, TStrings ADDINFO) {
  THmsScriptMediaItem Item;
  Item = HmsCreateMediaItem('-VideoPreview'+IntToStr(gnTotalItems), PodcastItem.ItemID);
  Item[mpiTitle     ] = HmsHtmlToText(sName);
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/vids/info.jpg';
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0)); gnTotalItems++;
  Item[mpiTimeLength] = '00:00:20.000';
  
  string sInfo = '';
  if (ADDINFO.Values['Year'     ]) sInfo += "<c:#FFC3BD>Год: </c>"          +ADDINFO.Values["Year"     ]  + " ";
  if (ADDINFO.Values['Country'  ]) sInfo += "<c:#FFC3BD>Страна: </c>"       +ADDINFO.Values["Country"  ]  + "|";
  if (ADDINFO.Values['Infos'    ]) sInfo += "<c:#FFC3BD>Инфо: </c>"         +ADDINFO.Values["Infos"    ]  + "|";
  if (ADDINFO.Values['Rating'   ]) sInfo += "<c:#FFC3BD>Рейтинг: </c>"      +ADDINFO.Values["Rating"   ]  + "|";
  if (ADDINFO.Values['Director' ]) sInfo += "<c:#FFC3BD>Режиссёр: </c>"     +ADDINFO.Values["Director" ]  + "|";
  if (ADDINFO.Values['Time'     ]) sInfo += "<c:#FFC3BD>Длительность: </c>" +ADDINFO.Values["Time"     ]  + "|";
  if (ADDINFO.Values['Actors'   ]) sInfo += "<c:#FFC3BD>В ролях: </c>"      +ADDINFO.Values["Actors"   ]  + "|";
  if (ADDINFO.Values['Translate']) sInfo += "<c:#FFC3BD>Перевод: </c>"      +ADDINFO.Values["Translate"]  + "|";
  if (ADDINFO.Values['New'      ]) sInfo += "<c:#FFC3BD>Добавлено: </c>"    +ADDINFO.Values["New"      ]  + "|";
  if (ADDINFO.Values['Limit'    ]) sInfo += "<c:#FFC3BD>Ограничение: </c>"  +ADDINFO.Values["Limit"    ]  + "|";
  sInfo = Copy(sInfo, 1, Length(sInfo)-1);
  TStrings INFO = TStringList.Create();  
  INFO.Values['Poster'] = ADDINFO.Values['Poster'];
  INFO.Values['Title' ] = ADDINFO.Values['Title' ];
  INFO.Values['Info'  ] = Trim(sInfo);
  INFO.Values['Categ' ] = ADDINFO.Values['Genre' ];
  INFO.Values['Descr' ] = ADDINFO.Values['Descr' ];
  Item[1001001] = INFO.Text;
  INFO.Free();
}
// ----------------------------------------------------------------------------
void AddVideoInfoItems(TStrings ADDINFO) {
  if (ADDINFO.Values['Director' ]) CreateVideoInfoItem('Режиссёр: '            +ADDINFO.Values['Director'], ADDINFO); 
  if (ADDINFO.Values['Country'  ]) CreateVideoInfoItem('Страна: '              +ADDINFO.Values['Country' ], ADDINFO); 
  if (ADDINFO.Values['Genre'    ]) CreateVideoInfoItem('Жанр: '                +ADDINFO.Values['Genre'   ], ADDINFO); 
  if (ADDINFO.Values['Year'     ]) CreateVideoInfoItem('Год: '                 +ADDINFO.Values['Year'    ], ADDINFO);
  if (ADDINFO.Values['Actors'   ]) CreateVideoInfoItem('Актеры: '              +ADDINFO.Values['Actors'  ], ADDINFO);
  if (ADDINFO.Values['Time'     ]) CreateVideoInfoItem('Время фильма/серии: '  +ADDINFO.Values['Time'    ], ADDINFO);
  if (ADDINFO.Values['Infos'    ]) CreateVideoInfoItem('Info: '                +ADDINFO.Values['Infos'   ], ADDINFO);
  if (ADDINFO.Values['Rating'   ]) CreateVideoInfoItem('IMDB: '                +ADDINFO.Values['Rating'  ], ADDINFO);
  if (ADDINFO.Values['Quality'  ]) CreateVideoInfoItem('Kачествo: '            +ADDINFO.Values['Quality' ], ADDINFO);
  if (ADDINFO.Values['New'      ]) CreateVideoInfoItem('Добавлено: '           +ADDINFO.Values['New'     ], ADDINFO);
  if (ADDINFO.Values['Limit'    ]) CreateVideoInfoItem('Ограничение: '         +ADDINFO.Values['Limit'   ], ADDINFO);
}
///////////////////////////////////////////////////////////////////////////////
// ---- Создание ссылок на файл(ы) по переданной ссылке (шаблону) -------------
void CreateVideoLinks(THmsScriptMediaItem Folder, string sName, string sLink, bool bSeparateInFolders = false) {
  TStrings LIST = TStringList.Create(); // Создаём объект для хранения списка ссылок
  try {
    LIST.Text = ReplaceStr(sLink, ',', '\n'); // Загружаем ссылки разделённые запятой в список
    if (LIST.Count < 2) bSeparateInFolders = false;  // Если ссылок одна - то не рассовываем по пакам с именем качества
    for (int i=0; i < LIST.Count; i++) {
      string sUrl = LIST[i], sQual = '';             // Получаем очередную ссылку из списка
      HmsRegExMatch('(\\[.*?\\])(.*)', sUrl, sQual); // Получаем название качества
      if (bSeparateInFolders)                        // Если установлен флаг распихивания разного качества по папкам
        AddMediaItem(Folder, sName, sUrl, sQual);    // - то создаём ссылку в папке с названием качества
      else   {         // Иначе просто создаём ссылку, впереди названия которой будет указано качество
        sQual= ReplaceStr(sQual, '[1080 HD]', '[1080HD]');
        AddMediaItem(Folder, Trim(sQual+' '+sName), sUrl);
      }
    }
  } finally { LIST.Free; } // Освобождаем объект из памяти
}

///////////////////////////////////////////////////////////////////////////////
// ---- Создание серий из плейлиста -------------------------------------------
void CreateSeriesFromPlaylist(THmsScriptMediaItem Folder, string sLink, string sName='') {
  string sData,sLink1, sLink2,sLink3,gsIDBase, s1, s2, s3; int i; TJsonObject JSON, PLITEM, INFO; TJsonArray PLAYLIST, INFOLIST; // Объявляем переменные
  int nSeason=0, nEpisode=0; string sVal; THmsScriptMediaItem Item;
  
  // Если передано имя плейлиста, то создаём папку, в которой будем создавать элементы
  if (Trim(sName)!='') Folder = Folder.AddFolder(sName);
  
  // Если в переменной sLink сожержится знак '{', то там не ссылка, а сами данные Json
  if (Pos('{', sLink)>0) {
    sData = sLink;
  } else {
    sData = HmsDownloadURL(sLink, "Referer: "+gsHeaders); // Загружаем плейлист
    if (LeftCopy(sData, 1)=="#")
      sData = BaseDecode(sData);                // Дешифруем
  }  
  
  INFO = TJsonObject.Create();
  JSON = TJsonObject.Create();                  // Создаём объект для работы с Json
  try {
    INFO.LoadFromString(gsSeriesInfo);          // Загрудаем информацию о названиях серий (если есть)
    JSON.LoadFromString(sData);                 // Загружаем json данные в объект
    PLAYLIST = JSON.AsArray;                    // Получаем объект как массив
    if (PLAYLIST!=nil) {                        // Если получили массив, то запускаем обход всех элементов в цикле
      for (i=0; i<PLAYLIST.Length; i++) {
        PLITEM = PLAYLIST[i];                   // Получаем текущий элемент массива
        sName = PLITEM.S['title'];
        sLink = PLITEM.S['file' ];              // Получаем значение ссылки на файл
        sName = HmsJsonDecode(sName);
        sName = HmsUtf8Decode(sName);           // В данном случае, у нас названия в UTF-8
        // Форматируем числовое представление серий в названии
        // Если в названии есть число, то будет в s1 - то, что стояло перед ним, s2 - само число, s3 - то, что было после числа
        if (HmsRegExMatch3('^(.*?)(\\d+)(.*)$', sName, s1, s2, s3)) {
          if (PLAYLIST.Length>99)
            sName = Format('%.3d %s %s', [StrToInt(s2), s1, s3]); // Форматируем имя - делаем число двухцифровое (01, 02...)
          else
            sName = Format('%.2d %s %s', [StrToInt(s2), s1, s3]); // Форматируем имя - делаем число двухцифровое (01, 02...)
        }
        
        // Получаем случайную ссылку из списка указанных (т.е. если встречаются "or" или "and")
        // sLink = ReplaceStr(ReplaceStr(sLink, " or ", "|"), " \\or ", "|");       // Заменяем все ключевые слова между ссылками на знак "|"
        //sLink = ExtractWord(Int(Random * WordCount(sLink, "|"))+1, sLink, "|");  // Получаем случайную строку из списка строк, разделённых знаком "|"
        
        // Если это серия сериала, пытаемся получить название серии из информации
        if (HmsRegExMatch2('s(\\d+)e(\\d+)', PLITEM.S['id'], s1, s2)) {
          if (INFO['message\\episodes\\'+s1]!=nil) {
            // Если есть инфа об этом сезоне - ищем наш номер эпизода
            INFOLIST = INFO['message\\episodes\\'+s1].AsArray;
            for (int e=0; e < INFOLIST.Length; e++) {
              if (INFOLIST[e].S['e']==s2) {
                // Нашли номер эпизода, получаем его название
                sName = HmsUtf8Decode(INFOLIST[e].S['n']);
                // Форматируем - ставим номер эпизода в начало названия
                if (PLAYLIST.Length>99)
                  sName = Format('%.3d '+Trim(sName), [StrToInt(s2)]);
                else
                  sName = Format('%.2d '+Trim(sName), [StrToInt(s2)]);
                break;
              }
            }
          }
        }
        
        // Проверяем, если это вложенный плейлист - запускаем создание элементов из этого плейлиста рекурсивно
        if (PLITEM.B['folder']) 
          CreateSeriesFromPlaylist(Folder, PLITEM.S['folder'], sName);
        else {
          CreateVideoLinks(Folder, sName, sLink, true); // Это не плейлист - просто создаём ссылки на видео
        }
      }
    } // end if (PLAYLIST!=nil) 
    
  } finally { JSON.Free; INFO.Free; } // Какие бы ошибки не случились, освобождаем объект из памяти
}

//////////////////////////////////////////////////////////////////////////////
// POST запрос для метки П с переданными параметрами
string FilmixmAPI(string sPost) {
  string sData, sRet;
  int nPort = 443; if (Pos('--https', mpPodcastParameters)<1) nPort = 80;
  sData = HmsSendRequestEx('filmix.co', '/api/v2/movie/view_time', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsUrlBase, sPost, nPort, 0x10, sRet, true);
  return sData;
}

// Пометка просмотренной серии 
string MarkView(string sID) {
  if (sID!='')
    FilmixmAPI("add_item="+sID+"&time=0");
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Декодирование Base64 с предворительной очисткой от мусора
string BaseDecode(string sData) {
  string sPost,sDomen,Garbage1,Garbage2,Garbage3,Garbage4,Garbage5;
  HmsRegExMatch('#2(.*)', sData, sData);
  if(Garbage1==''){
    sPost = 'http://moon.cx.ua/filmix/key.php';
    sDomen = HmsDownloadURL(sPost, 'Referer: '+mpFilePath, true);
  }
  HmsRegExMatch('"1":\\s"([^"]+)"' , sDomen, Garbage1);
  HmsRegExMatch('"2":\\s"([^"]+)"' , sDomen, Garbage2);
  HmsRegExMatch('"3":\\s"([^"]+)"' , sDomen, Garbage3);
  HmsRegExMatch('"4":\\s"([^"]+)"' , sDomen, Garbage4);
  HmsRegExMatch('"5":\\s"([^"]+)"' , sDomen, Garbage5);
  // Убираем перечисленный в массиве мусор за несколько проходов, ибо один мусор может быть разбавлен в середине другим  
  Variant tr=[Garbage1,Garbage2,Garbage3,Garbage4,Garbage5];
  for (int n=0; n<3; n++) for (int i=0; i<Length(tr); i++) sData = ReplaceStr(sData, tr[i], '');
  string sResult = HmsBase64Decode(sData);
  return sResult;
}


//////////////////////////////////////////////////
/////////Создание ссылки на трейлер////////////////////////////
void LinkTrailer (string sLink){
  string sResourceLink, sHtml, sData_Id, sData,sServ, sVal, sJson;
  if (HmsRegExMatch(gsUrlBase+'/.*?/(\\d+)/trailers', mpFilePath, sData_Id))
    HmsRegExMatch('//(.*)', gsUrlBase, sServ);
  int nPort = 80; if (gbHttps) nPort = 443;
  sData = HmsSendRequestEx(sServ, '/api/movies/player_data', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, 'post_id='+sData_Id, nPort, 16, sVal, true);
  //sData = HmsJsonDecode(sData); 
  HmsRegExMatch('"trailers":\\s*\\{\\s*".*?":\\s*"(.*?)"', sData, sJson);
  if (sJson == '') {HmsLogMessage(1, "Ошибка! Трейлер не доступен, или его нет на сайте!"); return;}
  sResourceLink = BaseDecode(sJson);
  
  
  string sQual;
  if (HmsRegExMatch(',\\[\\d+\\D+\\].*,\\[\\d+\\D+\\](.*)', sResourceLink, sQual)) { 
    MediaResourceLink = sQual;
  }else if (HmsRegExMatch(',\\[\\d+\\D+\\](.*)', sResourceLink, sQual)) { 
    MediaResourceLink = sQual;
  }
  
}


///////////////////////////////////////////////////////////////////////////////
// Добавление или удаление сериала из/в избранное
void AddRemoveFavorites() {
  string sLink, sCmd, sID; THmsScriptMediaItem FavFolder, Item, Src; bool bExist;
  
  if (!HmsRegExMatch3('(Add|Del)=(.*?);(.*)', mpFilePath, sCmd, sID, sLink)) return;
  FavFolder = HmsFindMediaFolder(Podcast.ItemID, 'favorites');
  if (FavFolder!=nil) {
    Item = HmsFindMediaFolder(FavFolder.ItemID, sLink);
    bExist = (Item != nil);
    if      ( bExist && (sCmd=='Del')) Item.Delete();
    else if (!bExist && (sCmd=='Add')) {
      Src = HmsFindMediaFolder(sID, sLink);
      if (Src!=nil) {
        Item = FavFolder.AddFolder(Src[mpiFilePath]);
        Item.CopyProperties(Src, [mpiTitle, mpiThumbnail, mpiJsonInfo, mpiKPID]);
      }
    }
  }
  MediaResourceLink = ProgramPath()+'\\Presentation\\Images\\videook.mp4';
  if (bExist) { mpTitle = "Добавить в избранное" ; mpFilePath = "-FavDel="+sLink; }
  else        { mpTitle = "Удалить из избранного"; mpFilePath = "-FavDel="+sLink; }
  PodcastItem[mpiTitle] = mpTitle;
  PodcastItem.ItemOrigin.ItemParent[mpiTitle] = mpTitle;
  PodcastItem[mpiFilePath] = mpFilePath;
  PodcastItem.ItemOrigin.ItemParent[mpiFilePath] = mpFilePath;
  HmsIncSystemUpdateID(); // Говорим устройству об обновлении содержания
}
///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на видео-файл или серии
void CreateLinks() {
  String sHtml, sData,sPls, sErr, sError, sName, sLink, sID, sVal, sServ, sPage, sPost, sKey;
  THmsScriptMediaItem Item; TStrings ADDINFO; TRegExpr RegExp; int i, nCount, n; TJsonObject JSON, TRANS,sPl;
  
  sHtml = HmsDownloadURL(mpFilePath, "Referer: "+gsHeaders);  // Загружаем страницу сериала
  sHtml = HmsUtf8Decode(HmsRemoveLineBreaks(sHtml));
  
  if (!HmsRegExMatch('data-id="(\\d+)"', sHtml, sID)) {
    HmsLogMessage(1, "Невозможно найти видео ID на странице фильма.");
    return;
  };
  
  HmsRegExMatch('//(.*)', gsUrlBase, sServ);
  if (HmsRegExMatch('--quality=(\\d+)', mpPodcastParameters, sVal)) gnQual = StrToInt(sVal);
  //POST https://filmix.co/api/episodes/get?post_id=103435&page=1  // episodes name
  //POST https://filmix.co/api/torrent/get_last?post_id=103435     // tottent file info
  
  // -------------------------------------------------
  // Собираем информацию о фильме
  ADDINFO = TstringList.Create();
  if (HmsRegExMatch('Время:(.*?)</span>\\s</div>', sHtml, sData)) {
    if (HmsRegExMatch('(\\d+)\\s+мин', ' '+sData, sVal)) {
      gnTime =  HmsTimeFormat(StrToInt(sVal)*60); // Из-за того что серии иногда длинее, добавляем пару минут
    }
    ADDINFO.Values['Time'   ] = gnTime;
   
  }
  
      HmsRegExMatch('/y(\\d{4})"', sHtml, ADDINFO.Values['Year']);
  if (HmsRegExMatch('(<a[^>]+genre.*?)</div>', sHtml, sVal)) ADDINFO.Values['Genre'] = HmsHtmlToText(sVal);
  if (HmsRegExMatch('<span class="added-info">(.*?)\\s+<i', sHtml, sVal)) ADDINFO.Values['New'] = HmsHtmlToText(sVal);
  if (HmsRegExMatch('<div class="about" itemprop="description"><div class="full-story">(.*?)</div>', sHtml, sVal)) ADDINFO.Values['Descr'] = HmsHtmlToText(sVal);
  if (HmsRegExMatch('<span class="video-in"><span>(.*?)</span></span>', sHtml, sVal)) ADDINFO.Values['Infos'] = HmsHtmlToText(sVal);;
  if (HmsRegExMatch('Страна:(.*?)</div'   , sHtml, sVal)) ADDINFO.Values['Country'] = HmsHtmlToText(sVal);
  if (HmsRegExMatch('Режиссер:(.*?)</div' , sHtml, sVal)) ADDINFO.Values['Director'] = HmsHtmlToText(sVal);
  if (HmsRegExMatch('Перевод:(.*?)</div'  , sHtml, sVal)) ADDINFO.Values['Translate'] = (HmsHtmlToText(sVal));
  if (HmsRegExMatch('В ролях:(.*?)</div'  , sHtml, sVal)) ADDINFO.Values['Actors'] = (HmsHtmlToText(sVal));
  if (HmsRegExMatch('MPAA:(.*?)</div'     , sHtml, sVal)) ADDINFO.Values['Limit'] = (HmsHtmlToText(sVal));
  if (HmsRegExMatch('(<div[^>]+quality.*?)</div'  , sHtml, sVal)) ADDINFO.Values['Quality'] = (HmsHtmlToText(sVal));
  if (HmsRegExMatch2('<span[^>]+imdb.*?<p>(.*?)</p>.*?<p>(.*?)</p>', sHtml, sName, sVal)) {
    if ((sName!='-') && (sName!='0')) ADDINFO.Values['Rating'] = (sName+" ("+sVal+")");
  }
  if (ADDINFO.Values['Poster']=='') ADDINFO.Values['Poster'] = mpThumbnail;
  if (ADDINFO.Values['Title' ]=='') ADDINFO.Values['Title' ] = mpTitle;
  // -------------------------------------------------
  
  int nPort = 80; if (gbHttps) nPort = 443;
  
  gsSeriesInfo = HmsSendRequestEx(sServ, '/api/episodes/get', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, 'post_id='+sID, nPort, 16, '', true);  
  
  sData = HmsSendRequestEx(sServ, '/api/movies/player_data', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, 'post_id='+sID, nPort, 16, sVal, true);
  
  sErr = HmsDownloadURL('http://moon.cx.ua/filmix/new.php?ref='+HmsBase64Encode(mpFilePath)+'&id='+sID, "Referer: "+gsHeaders);  // Загружаем страницу сериала
  HmsRegExMatch('"err":"([^"]+)"', sErr, sError); 
  sError = HmsJsonDecode(sError);
  
  JSON  = TJsonObject.Create();
  try {
    JSON.LoadFromString(sData);
    sPl = JSON['message\\translations'];
    sPls = sPl.S['pl'];
    TRANS  = JSON['message\\translations\\video'];
    nCount = TRANS.Count; // Количество озвучек
    for (i=0; i<nCount; i++) {
      sName = TRANS.Names[i];
      sLink = TRANS.S[sName];
      sName = HmsUtf8Decode(sName);
      sLink = HmsExpandLink(BaseDecode(sLink), gsUrlBase);
      //sLink  = GetRandomServerFile(sLink);
      // Проверяем, ссылка эта на плейлист?
      if (HmsRegExMatch('/pl/', sLink, '')) {
        if (nCount > 1) {
          // Количество озвучек больше одного - создаём просто папки с названием озвучки
          Item = CreateFolder(sName, sLink);
          Item[mpiSeriesInfo] = gsSeriesInfo;
        } else
          // Иначе создаём ссылки на серии из плейлиста
        CreateSeriesFromPlaylist(PodcastItem, sLink);
        
      } else {
        // Это не плейлист - просто создаём ссылку на видео
        CreateVideoLinks(PodcastItem, sName, sLink);
        
      }
    }
    
    
    if (nCount==0) ErrorItem(sError);
    
    // Специальная папка добавления/удаления в избранное в доп. парамертах добавить ключ --controlfavorites
   
    if ((sPls=="yes") &&  (Pos('--controlfavorites', mpPodcastParameters)>0)) {
      // Проверка, находится ли сериал в избранном?
      Item = HmsFindMediaFolder(Podcast.ItemID, 'favorites');
      if (Item!=nil) {
        bool bExist = HmsFindMediaFolder(Item.ItemID, mpFilePath) != nil;
        if (bExist) { sName = "Удалить из избранного"; sLink = "-FavDel="+PodcastItem.ItemParent.ItemID+";"+mpFilePath; }
        else        { sName = "Добавить в избранное" ; sLink = "-FavAdd="+PodcastItem.ItemParent.ItemID+";"+mpFilePath; }
        CreateMediaItem(PodcastItem, sName, sLink, 'http://wonky.lostcut.net/icons/ok.png', 1, sName);
      }
    } 
   
  } finally { JSON.Free; }
  
  // Добавляем ссылку на трейлер, если есть
  if (HmsRegExMatch('data-id="trailers"><a[^>]+href="(.*?)"', sHtml, sLink)) {
     Item = AddMediaItem(PodcastItem, 'Трейлер', sLink); // Это не плейлист - просто создаём ссылки на видео
     gnTime = "00:04:00.000";
     sVal = '4';
     gnTime =  HmsTimeFormat(StrToInt(sVal)*60)+'.000';
     Item[mpiTimeLength] = gnTime;
     
  }

   
   

   // Создаём информационные элементы (если указан ключ --videoinfoitems в дополнительных параметрах)
  
  if (HmsRegExMatch('--vinfo', mpPodcastParameters, sVal)) {
    if (Pos('--videoinfoitems', mpPodcastParameters)>0) AddVideoInfoItems(ADDINFO);
    else CreateVideoInfoItem('Информация о видео', ADDINFO);
  }
  
  
 
  ///////////////////////////////////////////////////////////////////////////////
  
  ADDINFO.Free();
  
} //end CreateLinks

///////////////////////////////////////////////////////////////////////////////
//                      Г Л А В Н А Я   П Р О Ц Е Д У Р А                    //
///////////////////////////////////////////////////////////////////////////////
{
  HmsRegExMatch('^(.*?//[^/]+)', Podcast[mpiFilePath], gsUrlBase); // Получаем значение в gsUrlBase
  gbHttps = (LeftCopy(gsUrlBase, 5)=='https');                     // Флаг использования 443 порта для запросов
  gsHeaders += ':authority: '+gsUrlBase+'\r\nOrigin: '+gsUrlBase+'\r\n';
  
  if (PodcastItem.IsFolder) {
    if (!LoginToFilmix()) return;
    PodcastItem.DeleteChildItems;
    // Если это папка, создаём ссылки внутри этой папки
    if (HmsRegExMatch('/pl/', mpFilePath, '')) {
      gsSeriesInfo = PodcastItem[mpiSeriesInfo];
      CreateSeriesFromPlaylist(PodcastItem, mpFilePath);
    } else
      CreateLinks();
  
  } else {
    if (LeftCopy(mpFilePath, 13)=='-VideoPreview') {VideoPreview();  return;}
    else if (LeftCopy(mpFilePath, 4)=='-Fav') {AddRemoveFavorites(); return;}
    else if (HmsRegExMatch('/(trejlery|trailers)', mpFilePath, '')) {
                                               LinkTrailer(mpFilePath); return;
   } else {
                                  
    if ((Pos('--markonplay', mpPodcastParameters)>0) && (Pos('[П]', mpTitle)<1)) {
     THmsScriptMediaItem Item;  string sHtml, sVal, sName ;
    // Придёться загрузить страницу сериала. Ищем ссылку на сам сериал.
    Item = PodcastItem;
    while ((Item!=nil) && (Pos("/filmi/", Item[mpiFilePath])<1) && (Pos("/seria/", Item[mpiFilePath])<1)) Item = Item.ItemParent;
    if (Item!=nil) {
      // Загружаем страницу сериала
      sHtml = HmsDownloadURL(Item[mpiFilePath], "Referer: "+gsHeaders, true);
      // Получаем значение session
      HmsRegExMatch('data-id-post="(\\d+)"', sHtml, sVal); 
      // Почечаем серию как просмотренную
      MarkView(sVal);
    }
    
    
    if (HmsRegExMatch2('(\\d+)\\s+(.*)', mpTitle, sVal, sName)) {
      PodcastItem.ItemOrigin[mpiTitle] = Format('%.2d [П] %s', [StrToInt(sVal), sName]);
      HmsIncSystemUpdateID();
    } else {
      HmsRegExMatch2('(\\[\\d+\\D+\\])\\s+(.*)', mpTitle, sVal, sName);
      PodcastItem.ItemOrigin[mpiTitle] = Format('%s [П] %s', [sVal, sName]);
                                                   HmsIncSystemUpdateID();
           }
         }
                                                
      }
    MediaResourceLink = mpFilePath;
  }
  
}
  
