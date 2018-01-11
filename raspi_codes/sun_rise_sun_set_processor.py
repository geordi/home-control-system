import datetime

class SunRiseSunSetDB:

    def __init__(self, db_filename):
        self.db_filename = db_filename
        self.db = self.preprocess()

    def process_header(self, line):
        year = line.strip()[1:]
        return int(year)

    def preprocess(self):
        db = {}

        with open(self.db_filename) as f:
            year = datetime.datetime.now().year
            for line in f:
                #day, jan, feb, mar, apr, may, jun, jul, aug, sep, oct, nov, dec = line.rstrip().split(';')
                one_day_in_months = line.rstrip().split(';')
                day_in_month = int(one_day_in_months[0])
                one_day_in_months = one_day_in_months[1:]

                for no, day in enumerate(one_day_in_months):
                    if day[0].isdigit():
                        sun_rise, sun_set = day.split('-')
                        record_date = datetime.date(year, no+1, day_in_month)
                        
                        sun_rise_h, sun_rise_m = int(sun_rise[:2]), int(sun_rise[2:])

                        record_sun_rise = datetime.datetime(year, no+1, day_in_month, sun_rise_h, sun_rise_m)

                        sun_set_h, sun_set_m = int(sun_set[:2]), int(sun_set[2:])
                        record_sun_set = datetime.datetime(year, no+1, day_in_month, sun_set_h, sun_set_m)

                        db[record_date] = {'rise': record_sun_rise, 'set': record_sun_set}

        return db

    def sun_rise(self, req_date):
        return self.db[req_date]['rise']

    def sun_set(self, req_date):
        return self.db[req_date]['set']

    
def main():
    db = SunRiseSunSetDB('/home/pi/sun_rise_set.txt')

    
    for rec in db.db:
        print(rec, db.db[rec])

if __name__ == '__main__':
    main()
