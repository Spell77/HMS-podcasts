// 2018.12.16  Collaboration: WendyH, Big Dog, михаил, Spell
//////////////////  Получение ссылок на медиа-ресурс   ////////////////////////
#define mpiSeriesInfo 10323  // Идентификатор для хранения информации о сериях

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase    = ''; // Url база ссылок нашего сайта (берётся из корневого элемента)
bool      gbHttps      = (LeftCopy(gsUrlBase, 5)=='https');
string      gnTime       = '01:40:00.000';
int       gnTotalItems = 0;
int       gnQual       = 0;  // Минимальное качество для отображения
TDateTime gStart       = Now;
string    gsSeriesInfo = ''; // Информация о сериях сериала (названия)
string    gsIDBase     = '';
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
//mpiFilePath
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
    //ErrorItem('Не указан логин или пароль');
    //return false;
    return true; // Не включена авторизация - работаем так, без неё.
  }
  
  sName = HmsHttpEncode(HmsUtf8Encode(mpPodcastAuthorizationUserName)); // Логин
  sPass = HmsHttpEncode(HmsUtf8Encode(mpPodcastAuthorizationPassword)); // Пароль
  sPost = 'login_name='+sName+'&login_password='+sPass+'&login_not_save=1&login=submit';
  //sData = HmsSendRequestEx(sDomen, '/engine/ajax/user_auth.php', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, sPost, nPort, nFlags, sRet, true);
  sData = HmsSendRequestEx(sDomen, '/', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, sPost, nPort, nFlags, sRet, true);
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

///////////////////////Получение случайного варианта сервера из предлагаемых/////////
string GetRandomServerFile(string sLink) {
  string sVal;
  sLink = ReplaceStr(sLink, " or "     , "|");
  sLink = ReplaceStr(sLink, " \\or "   , "|");
  sLink = ReplaceStr(sLink, " or\\s* " , "|");
  sLink = ExtractWord(int(Random()*WordCount(sLink, "|"))+1, sLink, "|");
  return sLink;
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
      else                                           // Иначе просто создаём ссылку, впереди названия которой будет указано качество
        AddMediaItem(Folder, Trim(sQual+' '+sName), sUrl);
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
        sLink = ReplaceStr(ReplaceStr(sLink, " or ", "|"), " \\or ", "|");       // Заменяем все ключевые слова между ссылками на знак "|"
        sLink = ExtractWord(Int(Random * WordCount(sLink, "|"))+1, sLink, "|");  // Получаем случайную строку из списка строк, разделённых знаком "|"
        
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Декодирование Base64 с предворительной очисткой от мусора
string BaseDecode(string sData) {
  string sPost,sDomen,Garbage1,Garbage2,Garbage3,Garbage4,Garbage5;
  HmsRegExMatch('#2(.*)', sData, sData);
  if(Garbage1==''){
    sPost = 'https://moon.cx.ua/filmix/key.php';
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

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на видео-файл или серии
void CreateLinks() {
  String sHtml, sData, sErr, sError, sName, sLink, sID, sVal, sServ, sPage, sPost, sKey;
  THmsScriptMediaItem Item; TRegExpr RegExp; int i, nCount, n; TJsonObject JSON, TRANS;
  
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
  if (HmsRegExMatch('Время:(.*?)</span>\\s</div>', sHtml, sData)) {
    if (HmsRegExMatch('(\\d+)\\s+мин', ' '+sData, sVal)) {
      gnTime =  HmsTimeFormat(StrToInt(sVal)*60)+'.000'; // Из-за того что серии иногда длинее, добавляем пару минут
    }
    PodcastItem[mpiTimeLength] = gnTime;
    
    
  }
  HmsRegExMatch('/y(\\d{4})"', sHtml, PodcastItem[mpiYear]);
  if (HmsRegExMatch('(<a[^>]+genre.*?)</div>', sHtml, sVal)) PodcastItem[mpiGenre] = HmsHtmlToText(sVal);
  if (HmsRegExMatch('<div class="about" itemprop="description"><div class="full-story">(.*?)</div>', sHtml, sVal)) PodcastItem[mpiComment] = HmsHtmlToText(sVal);
  HmsRegExMatch('<span class="video-in"><span>(.*?)</span></span>', sHtml, PodcastItem[mpiAuthor]);
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
    
  } finally { JSON.Free; }
  
  // Добавляем ссылку на трейлер, если есть
  if (HmsRegExMatch('data-id="trailers"><a[^>]+href="(.*?)"', sHtml, sLink)) {
     Item = AddMediaItem(PodcastItem, 'Трейлер', sLink); // Это не плейлист - просто создаём ссылки на видео
     gnTime = "00:04:00.000";
     sVal = '4';
     gnTime =  HmsTimeFormat(StrToInt(sVal)*60)+'.000';
     Item[mpiTimeLength] = gnTime;
     
  }

  // Создаём информационные элементы (если указан ключ --addinfoitems в дополнительных параметрах)
  if (Pos('--addinfoitems', mpPodcastParameters) > 0) {
    if (HmsRegExMatch('(<div[^>]+contry.*?)</div'   , sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch('(<div[^>]+directors.*?)</div', sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch('(Жанр:</span>.*?)</div'      , sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch('(<div[^>]+translate.*?)</div', sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch('(<div[^>]+quality.*?)</div'  , sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if(PodcastItem[mpiTimeLength]!='') {sName = 'Время: ' +PodcastItem[mpiTimeLength];  AddInfoItem(HmsHtmlToText(sName));}
    sName = 'Описание: ' +PodcastItem[mpiComment];  AddInfoItem(HmsHtmlToText(sName));
    if(PodcastItem[mpiAuthor]){
    sName = 'Info: ' +PodcastItem[mpiAuthor];  AddInfoItem(HmsHtmlToText(sName));
   }
    if (HmsRegExMatch2('<span[^>]+imdb.*?<p>(.*?)</p>.*?<p>(.*?)</p>', sHtml, sName, sVal)) {
      if ((sName!='-') && (sName!='0')) AddInfoItem("IMDB: "+sName+" ("+sVal+")");
    }
  }
}

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
    // Если это запустили файл на просмотр, присваиваем MediaResourceLink значение ссылки на видео-файл 
     if (HmsRegExMatch('/(trejlery|trailers)', mpFilePath, '')) {
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
      if (HmsRegExMatch(',\\[\\d+\\D+\\].*?,\\[\\d+\\D+\\](.*)', sResourceLink, sQual)) { 
        MediaResourceLink = sQual;
      }else if (HmsRegExMatch(',\\[\\d+\\D+\\](.*)', sResourceLink, sQual)) { 
        MediaResourceLink = sQual;
      }
      
    } else
      MediaResourceLink = mpFilePath;
    
  }
}
      
      
      
      
     
