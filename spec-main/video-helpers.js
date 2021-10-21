/* Https://github.com/antimatter15/whammy麻省理工学院许可(麻省理工学院)版权(C)2015年郭凯文特此免费授予任何获得本软件副本及相关文档文件(下称“本软件”)的人使用本软件的权利，包括但不限于使用、复制、修改、合并、发布、分发、再许可和/或销售本软件副本的权利，以及允许获得本软件的人员这样做的权利。符合以下条件：上述版权声明和本许可声明应包含在本软件的所有副本或主要部分中。该软件是“按原样”提供的，没有任何形式的明示或默示保证，包括但不限于适销性、适合某一特定用途和不侵权的保证。在任何情况下，作者或版权所有者均不对因软件或软件的使用或其他交易而引起、产生或与之相关的任何索赔、损害赔偿或其他责任，无论是在合同诉讼、侵权诉讼或其他诉讼中承担责任。*/

function atob (str) {
  return Buffer.from(str, 'base64').toString('binary');
}

// 在这种情况下，帧有一个非常具体的含义，它将是。
// 一旦我写完代码就会详细说明。

function ToWebM (frames) {
  const info = checkFrames(frames);

  // 群集的最长持续时间(毫秒)。
  const CLUSTER_MAX_DURATION = 30000;

  const EBML = [
    {
      id: 0x1a45dfa3, // EBML。
      data: [
        {
          data: 1,
          id: 0x4286 // EBMLVersion。
        },
        {
          data: 1,
          id: 0x42f7 // EBMLReadVersion。
        },
        {
          data: 4,
          id: 0x42f2 // EBMLMaxIDLength。
        },
        {
          data: 8,
          id: 0x42f3 // EBMLMaxSizeLength。
        },
        {
          data: 'webm',
          id: 0x4282 // 文档类型。
        },
        {
          data: 2,
          id: 0x4287 // DocTypeVersion。
        },
        {
          data: 2,
          id: 0x4285 // DocTypeReadVersion。
        }
      ]
    },
    {
      id: 0x18538067, // 细分市场。
      data: [
        {
          id: 0x1549a966, // 信息。
          data: [
            {
              data: 1e6, // 以毫秒为单位做事情(持续时间刻度的纳秒数)。
              id: 0x2ad7b1 // 时间码比例。
            },
            {
              data: 'whammy',
              id: 0x4d80 // MuxingApp。
            },
            {
              data: 'whammy',
              id: 0x5741 // WritingApp。
            },
            {
              data: doubleToString(info.duration),
              id: 0x4489 // 持续时间。
            }
          ]
        },
        {
          id: 0x1654ae6b, // 轨道。
          data: [
            {
              id: 0xae, // TrackEntry。
              data: [
                {
                  data: 1,
                  id: 0xd7 // 跟踪号。
                },
                {
                  data: 1,
                  id: 0x73c5 // TrackUID。
                },
                {
                  data: 0,
                  id: 0x9c // FlagLacing。
                },
                {
                  data: 'und',
                  id: 0x22b59c // 语言。
                },
                {
                  data: 'V_VP8',
                  id: 0x86 // 编解码器ID。
                },
                {
                  data: 'VP8',
                  id: 0x258688 // 编码名称。
                },
                {
                  data: 1,
                  id: 0x83 // 轨迹类型。
                },
                {
                  id: 0xe0, // 视频。
                  data: [
                    {
                      data: info.width,
                      id: 0xb0 // 像素宽度。
                    },
                    {
                      data: info.height,
                      id: 0xba // 像素高度。
                    }
                  ]
                }
              ]
            }
          ]
        },
        {
          id: 0x1c53bb6b, // 线索。
          data: [
            // 球杆插入点。
          ]
        }

        // 簇插入点。
      ]
    }
  ];

  const segment = EBML[1];
  const cues = segment.data[2];

  // 生成群集(最长持续时间)。
  let frameNumber = 0;
  let clusterTimecode = 0;
  while (frameNumber < frames.length) {
    const cuePoint = {
      id: 0xbb, // 提示点。
      data: [
        {
          data: Math.round(clusterTimecode),
          id: 0xb3 // CueTime。
        },
        {
          id: 0xb7, // CueTrackPositions。
          data: [
            {
              data: 1,
              id: 0xf7 // CueTrack。
            },
            {
              data: 0, // 在我们知道的时候填写。
              size: 8,
              id: 0xf1 // CueClusterPosition。
            }
          ]
        }
      ]
    };

    cues.data.push(cuePoint);

    const clusterFrames = [];
    let clusterDuration = 0;
    do {
      clusterFrames.push(frames[frameNumber]);
      clusterDuration += frames[frameNumber].duration;
      frameNumber++;
    } while (frameNumber < frames.length && clusterDuration < CLUSTER_MAX_DURATION);

    let clusterCounter = 0;
    const cluster = {
      id: 0x1f43b675, // 聚类。
      data: [
        {
          data: Math.round(clusterTimecode),
          id: 0xe7 // 时码。
        }
      ].concat(clusterFrames.map(function (webp) {
        const block = makeSimpleBlock({
          discardable: 0,
          frame: webp.data.slice(4),
          invisible: 0,
          keyframe: 1,
          lacing: 0,
          trackNum: 1,
          timecode: Math.round(clusterCounter)
        });
        clusterCounter += webp.duration;
        return {
          data: block,
          id: 0xa3
        };
      }))
    };

    // 将群集添加到细分市场。
    segment.data.push(cluster);
    clusterTimecode += clusterDuration;
  }

  // 计算群集位置的第一遍。
  let position = 0;
  for (let i = 0; i < segment.data.length; i++) {
    if (i >= 3) {
      cues.data[i - 3].data[1].data[1].data = position;
    }
    const data = generateEBML([segment.data[i]]);
    position += data.size || data.byteLength || data.length;
    if (i !== 2) { // 不是线索。
      // 保存结果以避免对所有内容进行两次编码。
      segment.data[i] = data;
    }
  }

  return generateEBML(EBML);
}

// 将所有帧的长度相加，得到持续时间，喔。

function checkFrames (frames) {
  const width = frames[0].width;
  const height = frames[0].height;
  let duration = frames[0].duration;
  for (let i = 1; i < frames.length; i++) {
    if (frames[i].width !== width) throw new Error('Frame ' + (i + 1) + ' has a different width');
    if (frames[i].height !== height) throw new Error('Frame ' + (i + 1) + ' has a different height');
    if (frames[i].duration < 0 || frames[i].duration > 0x7fff) throw new Error('Frame ' + (i + 1) + ' has a weird duration (must be between 0 and 32767)');
    duration += frames[i].duration;
  }
  return {
    duration: duration,
    width: width,
    height: height
  };
}

function numToBuffer (num) {
  const parts = [];
  while (num > 0) {
    parts.push(num & 0xff);
    num = num >> 8;
  }
  return new Uint8Array(parts.reverse());
}

function numToFixedBuffer (num, size) {
  const parts = new Uint8Array(size);
  for (let i = size - 1; i >= 0; i--) {
    parts[i] = num & 0xff;
    num = num >> 8;
  }
  return parts;
}

function strToBuffer (str) {
  // 返回新的Blob([str])；

  const arr = new Uint8Array(str.length);
  for (let i = 0; i < str.length; i++) {
    arr[i] = str.charCodeAt(i);
  }
  return arr;
  // 这个更慢。
  // 返回新的Uint8Array(str.plit(‘’).map(function(E){。
  // 返回e.charCodeAt(0)。
  // }))。
}

// 对不起，这很难看，而且很难确切地理解为什么要这样做。
// 完全是真的，但原因是下面有一些代码，我并不是真的。
// 感觉像是理解了，这比用我的大脑更容易。

function bitsToBuffer (bits) {
  const data = [];
  const pad = (bits.length % 8) ? (new Array(1 + 8 - (bits.length % 8))).join('0') : '';
  bits = pad + bits;
  for (let i = 0; i < bits.length; i += 8) {
    data.push(parseInt(bits.substr(i, 8), 2));
  }
  return new Uint8Array(data);
}

function generateEBML (json) {
  const ebml = [];
  for (let i = 0; i < json.length; i++) {
    if (!('id' in json[i])) {
      // 已编码的blob或byteArray。
      ebml.push(json[i]);
      continue;
    }

    let data = json[i].data;
    if (typeof data === 'object') data = generateEBML(data);
    if (typeof data === 'number') data = ('size' in json[i]) ? numToFixedBuffer(data, json[i].size) : bitsToBuffer(data.toString(2));
    if (typeof data === 'string') data = strToBuffer(data);

    const len = data.size || data.byteLength || data.length;
    const zeroes = Math.ceil(Math.ceil(Math.log(len) / Math.log(2)) / 8);
    const sizeStr = len.toString(2);
    const padded = (new Array((zeroes * 7 + 7 + 1) - sizeStr.length)).join('0') + sizeStr;
    const size = (new Array(zeroes)).join('0') + '1' + padded;

    // 我其实不太明白上面发生了什么，所以我不是真的。
    // 为了解决这个问题，我可能只会写一些令人毛骨悚然的东西。
    // 将该字符串转换为类似缓冲区的内容。

    ebml.push(numToBuffer(json[i].id));
    ebml.push(bitsToBuffer(size));
    ebml.push(data);
  }

  // 将ebml转换为数组。
  const buffer = toFlatArray(ebml);
  return new Uint8Array(buffer);
}

function toFlatArray (arr, outBuffer) {
  if (outBuffer == null) {
    outBuffer = [];
  }
  for (let i = 0; i < arr.length; i++) {
    if (typeof arr[i] === 'object') {
      // 一个数组。
      toFlatArray(arr[i], outBuffer);
    } else {
      // 一个简单的元素。
      outBuffer.push(arr[i]);
    }
  }
  return outBuffer;
}

function makeSimpleBlock (data) {
  let flags = 0;
  if (data.keyframe) flags |= 128;
  if (data.invisible) flags |= 8;
  if (data.lacing) flags |= (data.lacing << 1);
  if (data.discardable) flags |= 1;
  if (data.trackNum > 127) {
    throw new Error('TrackNumber > 127 not supported');
  }
  const out = [data.trackNum | 0x80, data.timecode >> 8, data.timecode & 0xff, flags].map(function (e) {
    return String.fromCharCode(e);
  }).join('') + data.frame;

  return out;
}

// 还有些东西是从微不足道的、令人敬畏的仪式上逐字摘录的？

function parseWebP (riff) {
  const VP8 = riff.RIFF[0].WEBP[0];

  const frameStart = VP8.indexOf('\x9d\x01\x2a'); // VP8关键帧以0x9d012a标题开始。
  const c = [];
  for (let i = 0; i < 4; i++) c[i] = VP8.charCodeAt(frameStart + 3 + i);

  // 下面的代码是从比特流规范逐字复制而来的。
  let tmp = (c[1] << 8) | c[0];
  const width = tmp & 0x3FFF;
  const horizontalScale = tmp >> 14;
  tmp = (c[3] << 8) | c[2];
  const height = tmp & 0x3FFF;
  const verticalScale = tmp >> 14;
  return {
    width: width,
    height: height,
    data: VP8,
    riff: riff
  };
}

// 我想我是在即兴表演，假装这是一些已知的。
// 我想用一个随意而巧妙的双关语来解释这个习语，但是自从。
// 我在谷歌上找不到任何符合这个习语的东西。
// 用法，我想这只是某种精神病的结果。
// 休息一下，这让我编了个双关语。好了，别再胡说八道了(啊哈。
// 某种形式的救援)，这个功能是从Weppy那里批发来的。

function parseRIFF (string) {
  let offset = 0;
  const chunks = {};

  while (offset < string.length) {
    const id = string.substr(offset, 4);
    chunks[id] = chunks[id] || [];
    if (id === 'RIFF' || id === 'LIST') {
      const len = parseInt(string.substr(offset + 4, 4).split('').map(function (i) {
        const unpadded = i.charCodeAt(0).toString(2);
        return (new Array(8 - unpadded.length + 1)).join('0') + unpadded;
      }).join(''), 2);
      const data = string.substr(offset + 4 + 4, len);
      offset += 4 + 4 + len;
      chunks[id].push(parseRIFF(data));
    } else if (id === 'WEBP') {
      // 使用(偏移量+8)跳过“WebP”后的“VP8”/“VP8L”/“VP8X”字段。
      chunks[id].push(string.substr(offset + 8));
      offset = string.length;
    } else {
      // 未知区块类型；推送整个有效负载。
      chunks[id].push(string.substr(offset + 4));
      offset = string.length;
    }
  }
  return chunks;
}

// 下面是一个小实用程序函数，它充当其他函数的实用程序。
// 基本上，唯一的目的是对“持续时间”进行编码，其编码方式为。
// 双精度(编码难度比整数大得多)。
function doubleToString (num) {
  return [].slice.call(
    new Uint8Array(
      (
        new Float64Array([num]) // 创建一个float64数组。
      ).buffer) // 提取数组缓冲区。
    , 0) // 将Uint8Array转换为常规数组。
    .map(function (e) { // 因为它是一个规则数组，所以我们现在可以使用map。
      return String.fromCharCode(e); // 分别对所有字节进行编码。
    })
    .reverse() // 更正字节字符顺序(假设它现在是小字符顺序)。
    .join(''); // 将神圣婚姻中的字节连接为一个字符串。
}

function WhammyVideo (speed, quality = 0.8) { // 更抽象的API。
  this.frames = [];
  this.duration = 1000 / speed;
  this.quality = quality;
}

/* *@param{string}frame*@param{number}[时长]。*/
WhammyVideo.prototype.add = function (frame, duration) {
  if (typeof duration !== 'undefined' && this.duration) throw new Error("you can't pass a duration if the fps is set");
  if (typeof duration === 'undefined' && !this.duration) throw new Error("if you don't have the fps set, you need to have durations here.");
  if (frame.canvas) { // CanvasRenderingContext2D。
    frame = frame.canvas;
  }
  if (frame.toDataURL) {
    // Frame=fra.toDataURL(‘image/webp’，this.quality)；
    // 快速存储图像数据，这样我们就不会阻塞CPU。在Compile方法中编码。
    frame = frame.getContext('2d').getImageData(0, 0, frame.width, frame.height);
  } else if (typeof frame !== 'string') {
    throw new Error('frame must be a a HTMLCanvasElement, a CanvasRenderingContext2D or a DataURI formatted string');
  }
  if (typeof frame === 'string' && !(/^data:image\/webp;base64,/ig).test(frame)) {
    throw new Error('Input must be formatted properly as a base64 encoded DataURI of type image/webp');
  }
  this.frames.push({
    image: frame,
    duration: duration || this.duration
  });
};

WhammyVideo.prototype.compile = function (callback) {
  const webm = new ToWebM(this.frames.map(function (frame) {
    const webp = parseWebP(parseRIFF(atob(frame.image.slice(23))));
    webp.duration = frame.duration;
    return webp;
  }));
  callback(webm);
};

export const WebmGenerator = WhammyVideo;
