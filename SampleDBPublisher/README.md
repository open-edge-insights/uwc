# Sample DB Publisher 

This is a sample python publisher which writes sample data into the influx database without getting involved with any of the UWC services.

## Steps to Run 

1. To run the sample publisher app select 8 option while running [02_provision_UWC.sh](../build_scripts/02_provision_UWC.sh).

2. Sample json payload is present in [./publisher.py](./publisher.py).

```yaml
      meta = {
       'dataPersist': 'true',
       'test_msg': 1234
       }

```
Note: 
   * Add 'true' or 'false' in dataPersist key with single quotes.
   * If the string is to be added in sample payload key then please add key in [Telegraf_devmode.conf](../../Telegraf/config/Telegraf/Telegraf_devmode.conf) json_string_fields section.

3. Follow the [uwc_README](../README.md) to run uwc.

4. Add the following relative path in any of [uwc/uwc_recipes](../uwc_recipes) file to run with different use cases.

```yaml
- uwc/Sample_Publisher
```
